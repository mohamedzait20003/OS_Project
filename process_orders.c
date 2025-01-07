#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define NUM_ORDERS 3           // Number of orders
#define SHM_KEY_INVENTORY 1234 // Shared memory key for inventory
#define SHM_KEY_SALES 5678     // Shared memory key for sales

pthread_mutex_t lock; // Mutex for shared memory and log synchronization

// Structure to represent an order
typedef struct {
    int order_id;
    int items_to_order;
    char log[256]; // Buffer to store the log message
} OrderTask;

// Function to process an order
void *process_order(void *arg) {
    OrderTask *task = (OrderTask *)arg;

    // Attach to shared memory
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
    if (*inventory >= task->items_to_order) {
        *inventory -= task->items_to_order;
        *sales += task->items_to_order;
        snprintf(task->log, sizeof(task->log),
                 "Order %d: Processing %d items.\n"
                 "Order %d: Remaining inventory: %d, Total sales: %d\n",
                 task->order_id, task->items_to_order,
                 task->order_id, *inventory, *sales);
    } else {
        snprintf(task->log, sizeof(task->log),
                 "Order %d: Not enough inventory to process %d items.\n",
                 task->order_id, task->items_to_order);
    }
    pthread_mutex_unlock(&lock);

    shmdt(inventory);
    shmdt(sales);

    sleep(1); // Simulate order processing time
    return NULL;
}

int main() {
    pthread_t threads[NUM_ORDERS];
    OrderTask tasks[NUM_ORDERS];
    pthread_mutex_init(&lock, NULL);

    printf("Processing Orders...\n");

    for (int i = 0; i < NUM_ORDERS; i++) {
        printf("Enter the number of items for Order %d: ", i + 1);
        scanf("%d", &tasks[i].items_to_order);
        tasks[i].order_id = i + 1;

        if (pthread_create(&threads[i], NULL, process_order, &tasks[i]) != 0) {
            perror("Failed to create thread");
            exit(1);
        }
    }

    for (int i = 0; i < NUM_ORDERS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("\n--- Order Logs ---\n");
    for (int i = 0; i < NUM_ORDERS; i++) {
        printf("%s", tasks[i].log);
    }

    pthread_mutex_destroy(&lock);
    return 0;
}
