#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <microhttpd.h>
#include <unistd.h>
#include <cjson/cJSON.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <signal.h>

#define PORT 8880
#define QUEUE_SIZE 10
#define SHM_KEY 1234

typedef struct {
    const char *url;
    cJSON *json_data;
    struct MHD_Connection *connection;
} Request;

typedef struct {
    Request requests[QUEUE_SIZE];
    int front;
    int rear;
} RequestQueue;

typedef struct {
    int enqueued;
} RequestContext;

RequestQueue *queue;
pthread_mutex_t *queue_mutex;
pthread_cond_t *queue_cond;
pid_t server_pid;

void enqueue(Request request) {
    pthread_mutex_lock(queue_mutex);
    queue->requests[queue->rear] = request;
    queue->rear = (queue->rear + 1) % QUEUE_SIZE;
    printf("Queued Requests: %d\n", queue->rear);
    pthread_cond_signal(queue_cond);
    pthread_mutex_unlock(queue_mutex);
}

Request dequeue() {
    pthread_mutex_lock(queue_mutex);
    while (queue->front == queue->rear) {
        pthread_cond_wait(queue_cond, queue_mutex);
    }
    Request request = queue->requests[queue->front];
    printf("Dequeued Request: %d\n", queue->front);
    queue->front = (queue->front + 1) % QUEUE_SIZE;
    pthread_mutex_unlock(queue_mutex);
    return request;
}

int is_queue_empty() {
    pthread_mutex_lock(queue_mutex);
    int empty = (queue->front == queue->rear);
    pthread_mutex_unlock(queue_mutex);
    return empty;
}

const char* generate_report() {
    // Generate report operation
    static char response[256];
    snprintf(response, sizeof(response), "<html><body>Generating report...</body></html>");
    return response;
}

const char* terminate_server() {
    // Terminate server operation
    static char response[256];
    snprintf(response, sizeof(response), "<html><body>Server is shutting down...</body></html>");
    kill(server_pid, SIGTERM); // Send termination signal to the server process
    return response;
}

const char* pause_server(){
    // Pause server operation
    static char response[256];
    snprintf(response, sizeof(response), "<html><body>Server is paused...</body></html>");
    kill(server_pid, SIGSTOP); // Send pause signal to the server process
    return response;
}

const char* resume_server(){
    // Resume server operation
    static char response[256];
    snprintf(response, sizeof(response), "<html><body>Server is resumed...</body></html>");
    kill(server_pid, SIGCONT); // Send resume signal to the server process
    return response;
}

enum MHD_Result request_handler(void *cls, struct MHD_Connection *connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls) {
    if (*con_cls == NULL) {
        RequestContext *context = malloc(sizeof(RequestContext));
        context->enqueued = 0;
        *con_cls = context;
        return MHD_YES;
    }

    RequestContext *context = *con_cls;
    cJSON *json_data = NULL;

    if (*upload_data_size != 0) {
        json_data = cJSON_Parse(upload_data);
        if (json_data == NULL) {
            return MHD_NO;
        }
    }

    // Handle the request, whether it has a JSON body or not
    if (*upload_data_size == 0 && context->enqueued == 0) {
        Request request = {url, json_data, connection};
        enqueue(request);
        context->enqueued = 1; // Mark the request as enqueued
    }

    *upload_data_size = 0;
    return MHD_YES;
}

void *worker_thread(void *arg) {
    while (1) {
        if (is_queue_empty()) {
            continue;
        } else {
            Request request = dequeue();
            printf("Processing request for %s\n", request.url);

            const char *response;
            if (strcmp(request.url, "/termination") == 0) {
                response = terminate_server();
            } else if(strcmp(request.url, "/pause") == 0) {
                response = pause_server();
            } else if(strcmp(request.url, "/continue") == 0) {
                response = resume_server();
            } else if(strcmp(request.url, "/generate_report") == 0) {
            } else {
                response = "<html><body>Unknown operation</body></html>";
            }

            struct MHD_Response *mhd_response = MHD_create_response_from_buffer(strlen(response), (void*)response, MHD_RESPMEM_MUST_COPY);
            if (mhd_response == NULL) {
                fprintf(stderr, "Failed to create response\n");
                continue;
            }

            const union MHD_ConnectionInfo *conn_info = MHD_get_connection_info(request.connection, MHD_CONNECTION_INFO_CONNECTION_FD);
            if (conn_info == NULL) {
                fprintf(stderr, "Connection is no longer valid for %s with connection: %p\n", request.url, request.connection);
                if (request.json_data != NULL) {
                    cJSON_Delete(request.json_data);
                }
                continue;
            } else {
                printf("Connection is valid for %s with connection: %p\n", request.url, request.connection);

                int ret = MHD_queue_response(request.connection, MHD_HTTP_OK, mhd_response);
                if (ret != MHD_YES) {
                    fprintf(stderr, "Failed to queue response for %s with connection: %p\n", request.url, request.connection);
                } else {
                    printf("Response sent for %s with connection: %p\n", request.url, request.connection);
                }

                MHD_destroy_response(mhd_response);

                if (request.json_data != NULL) {
                    cJSON_Delete(request.json_data);
                }
            }
        }
    }
    return NULL;
}

int main() {
    // Store the server process ID
    server_pid = getpid();

    // Initialize shared memory
    int shmid = shmget(SHM_KEY, sizeof(RequestQueue) + sizeof(pthread_mutex_t) + sizeof(pthread_cond_t), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("Shared memory access failed");
        exit(1);
    }

    void *shared_memory = shmat(shmid, NULL, 0);
    if (shared_memory == (void *)-1) {
        perror("Shared memory attachment failed");
        exit(1);
    }

    queue = (RequestQueue *)shared_memory;
    queue_mutex = (pthread_mutex_t *)(shared_memory + sizeof(RequestQueue));
    queue_cond = (pthread_cond_t *)(shared_memory + sizeof(RequestQueue) + sizeof(pthread_mutex_t));

    queue->front = 0;
    queue->rear = 0;

    // Initialize the mutex and condition variable
    pthread_mutexattr_t mutex_attr;
    pthread_condattr_t cond_attr;

    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(queue_mutex, &mutex_attr);

    pthread_condattr_init(&cond_attr);
    pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(queue_cond, &cond_attr);

    // Create worker threads
    pthread_t workers[4];
    for (int i = 0; i < 4; i++) {
        pthread_create(&workers[i], NULL, worker_thread, NULL);
    }

    // Start the server
    struct MHD_Daemon *daemon;
    daemon = MHD_start_daemon(MHD_NO_FLAG, PORT, NULL, NULL, &request_handler, NULL, MHD_OPTION_END);
    if (NULL == daemon) return 1;

    printf("Server running on port %d\n", PORT);

    // Keep the server running indefinitely
    while (1) {
        MHD_run(daemon);
    }

    MHD_stop_daemon(daemon);
    return 0;
}