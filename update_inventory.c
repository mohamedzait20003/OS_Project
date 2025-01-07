#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHM_KEY_INVENTORY 1234 // Shared memory key for inventory

int main() {
    int shmid_inventory = shmget(SHM_KEY_INVENTORY, sizeof(int), 0666);

    if (shmid_inventory < 0) {
        perror("Shared memory access failed");
        exit(1);
    }

    int *inventory = (int *)shmat(shmid_inventory, NULL, 0);

    if (inventory == (void *)-1) {
        perror("Shared memory attachment failed");
        exit(1);
    }

    printf("Enter the number of items to add to the inventory: ");
    int items;
    scanf("%d", &items);

    *inventory += items;
    printf("Inventory updated. Current inventory: %d\n", *inventory);

    shmdt(inventory);

    return 0;
}
