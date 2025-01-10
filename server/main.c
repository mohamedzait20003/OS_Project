// Libraries
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
#include <errno.h>
#include <sqlite3.h>

// Header Files
#include "Headers/Update_Inventory.h"
#include "Headers/View_Inventory.h"
#include "Headers/Process_Orders.h"
#include "Headers/Generate_Report.h"

// Macros
#define PORT 8880
#define QUEUE_SIZE 100
#define SHM_KEY 3445

typedef struct {
    const char *url;
    const char* method;
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

// Database
sqlite3 *db;

void enqueue(Request request) {
    pthread_mutex_lock(queue_mutex);
    char *json_string = cJSON_Print(request.json_data);
    queue->requests[queue->rear] = request;
    queue->rear = (queue->rear + 1) % QUEUE_SIZE;
    pthread_cond_signal(queue_cond);
    pthread_mutex_unlock(queue_mutex);
}

Request dequeue() {
    pthread_mutex_lock(queue_mutex);
    while (queue->front == queue->rear) {
        pthread_cond_wait(queue_cond, queue_mutex);
    }
    Request request = queue->requests[queue->front];
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

const char* terminate_server() {
    // Terminate server operation
    static char response[256];
    snprintf(response, sizeof(response), "<html><body>Server is shutting down...</body></html>");
    kill(server_pid, SIGTERM); // Send termination signal to the server process
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
            fprintf(stderr, "Failed to parse JSON data\n");
            return MHD_NO;
        }
    }

    // Handle the request, whether it has a JSON body or not
    if (context->enqueued == 0) {
        Request request = {url, method, json_data, connection};
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
            if(strcmp(request.method, "GET") == 0){
                if(strcmp(request.url, "/view_inventory") == 0) {
                    response = view_inventory();
                } else if(strcmp(request.url, "/generate_report")==0){
                    response = generate_report();
                }
                else {
                    response = "Unknown operation";
                }
                
            } else if(strcmp(request.method, "POST") == 0){
                if (strcmp(request.url, "/update_inventory") == 0) {
                    if (request.json_data == NULL) {
                        fprintf(stderr, "No JSON data received\n");
                        response = "No JSON data received";
                    } else {
                        cJSON *item = cJSON_GetObjectItem(request.json_data, "Item");
                        cJSON *amount = cJSON_GetObjectItem(request.json_data, "Amount");
                        
                        if (item == NULL || amount == NULL) {
                            fprintf(stderr, "Failed to get 'Item' or 'Amount' from JSON data\n");
                            response = "Invalid JSON data\n";
                        } else {
                            const char *item_value = cJSON_GetStringValue(item);
                            int amount_value = cJSON_GetNumberValue(amount);
                            
                            if (item_value == NULL || amount_value == 0) {
                                fprintf(stderr, "Failed to convert 'Item' or 'Amount' to appropriate types\n");
                                response = "Invalid number format\n";
                            } else {
                                response = UpdateInventory(item_value, amount_value);
                            }
                        }
                    }
                } else if(strcmp(request.url, "/process_orders") == 0){
                    if (request.json_data == NULL) {
                        fprintf(stderr, "No JSON data received\n");
                        response = "No JSON data received";
                    } else {
                        cJSON *client = cJSON_GetObjectItem(request.json_data, "Client");
                        cJSON *items = cJSON_GetObjectItem(request.json_data, "Items");
                        
                        if (items == NULL || client == NULL || !cJSON_IsArray(items)) {
                            fprintf(stderr, "Failed to get 'Items' array from JSON data\n");
                            response = "Invalid JSON data\n";
                        } else {
                            int items_count = cJSON_GetArraySize(items);
                            const char **item_names = malloc(items_count * sizeof(char *));
                            int *item_amounts = malloc(items_count * sizeof(int));

                            if(item_names == NULL || item_amounts == NULL){
                                fprintf(stderr, "Failed to allocate memory for items\n");
                                response = "Internal server error\n";
                            } else {
                                cJSON *item = NULL;
                                int index = 0;

                                cJSON_ArrayForEach(item, items){
                                    cJSON *item_name = cJSON_GetObjectItem(item, "Item");
                                    cJSON *item_value = cJSON_GetObjectItem(item, "Value");

                                    if(item_name == NULL || item_value == NULL){
                                        fprintf(stderr, "Failed to get 'Item' or 'Value' from JSON data\n");
                                        response = "Invalid JSON data\n";
                                        break;
                                    }

                                    const char *item_name_value = cJSON_GetStringValue(item_name);
                                    int item_value_value = cJSON_GetNumberValue(item_value);
                                    if(item_name_value == NULL || item_value_value == 0){
                                       fprintf(stderr, "Invalid item name or value\n");
                                        response = "Invalid Item format\n";
                                        break;
                                    }

                                    item_names[index] = item_name_value;
                                    item_amounts[index] = item_value_value;
                                    index++;
                                }

                                response = process_order(client->valuestring, item_names, item_amounts, items_count);

                                free(item_names);
                                free(item_amounts);
                            }
                        }
                    }
                } else {
                    response = "Unknown operation";
                }
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

    // Initialize the database
    int rc = sqlite3_open("Database/Database.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    fprintf(stderr, "Database opened successfully\n");

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
    sqlite3_close(db);
    return 0;
}