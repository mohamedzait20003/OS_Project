#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHM_KEY_INVENTORY 1234 // Shared memory key for inventory
#define SHM_KEY_SALES 5678     // Shared memory key for sales

int main() {
    // Create shared memory for inventory
    int shmid_inventory = shmget(SHM_KEY_INVENTORY, sizeof(int), 0666 | IPC_CREAT);
    if (shmid_inventory < 0) {
        perror("Shared memory creation for inventory failed");
        return 1;
    }

    int *inventory = (int *)shmat(shmid_inventory, NULL, 0);
    if (inventory == (void *)-1) {
        perror("Shared memory attachment for inventory failed");
        return 1;
    }
    *inventory = 100; // Initialize inventory

    // Create shared memory for sales
    int shmid_sales = shmget(SHM_KEY_SALES, sizeof(int), 0666 | IPC_CREAT);
    if (shmid_sales < 0) {
        perror("Shared memory creation for sales failed");
        return 1;
    }

    int *sales = (int *)shmat(shmid_sales, NULL, 0);
    if (sales == (void *)-1) {
        perror("Shared memory attachment for sales failed");
        return 1;
    }
    *sales = 0; // Initialize sales

    printf("Shared memory initialized.\n");
    printf("Inventory: %d, Sales: %d\n", *inventory, *sales);

    // Detach from shared memory
    shmdt(inventory);
    shmdt(sales);

    return 0;
}
