#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define NUM_COUNTERS 3
#define SHM_KEY_INVENTORY 1234
#define SHM_KEY_SALES 5678

pthread_mutex_t lock;

typedef struct {
    int counter_id;
    int items_to_sell;
    char log[256]; // Buffer to store the log message
} CounterTask;

void *checkout(void *arg) {
    CounterTask *task = (CounterTask *)arg;

    int shmid_inventory = shmget(SHM_KEY_INVENTORY, sizeof(int), 0666);
    int shmid_sales = shmget(SHM_KEY_SALES, sizeof(int), 0666);

    if (shmid_inventory < 0 || shmid_sales < 0) {
        perror("Shared memory access failed");
        exit(1);
    }

    int *inventory = (int *)shmat(shmid_inventory, NULL, 0);
    int *sales = (int *)shmat(shmid_sales, NULL, 0);

    if (inventory == (void *)-1 || sales == (void *)-1) {
        perror("Shared memory attachment failed");
        exit(1);
    }

    pthread_mutex_lock(&lock);
    if (*inventory >= task->items_to_sell) {
        *inventory -= task->items_to_sell;
        *sales += task->items_to_sell;
        snprintf(task->log, sizeof(task->log), 
                 "Counter %d: Selling %d items.\n"
                 "Counter %d: Remaining inventory: %d, Total sales: %d\n",
                 task->counter_id, task->items_to_sell, 
                 task->counter_id, *inventory, *sales);
    } else {
        snprintf(task->log, sizeof(task->log), 
                 "Counter %d: Not enough inventory to sell %d items.\n",
                 task->counter_id, task->items_to_sell);
    }
    pthread_mutex_unlock(&lock);

    shmdt(inventory);
    shmdt(sales);

    sleep(1);
    return NULL;
}

int main() {
    pthread_t threads[NUM_COUNTERS];
    CounterTask tasks[NUM_COUNTERS];
    pthread_mutex_init(&lock, NULL);

    printf("Starting Checkout Counters...\n");

    for (int i = 0; i < NUM_COUNTERS; i++) {
        printf("Enter the number of items to sell at Counter %d: ", i + 1);
        scanf("%d", &tasks[i].items_to_sell);
        tasks[i].counter_id = i + 1;

        if (pthread_create(&threads[i], NULL, checkout, &tasks[i]) != 0) {
            perror("Failed to create thread");
            exit(1);
        }
    }

    for (int i = 0; i < NUM_COUNTERS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("\n--- Transaction Logs ---\n");
    for (int i = 0; i < NUM_COUNTERS; i++) {
        printf("%s", tasks[i].log);
    }

    pthread_mutex_destroy(&lock);
    return 0;
}
