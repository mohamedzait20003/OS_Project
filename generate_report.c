#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHM_KEY_SALES 5678 // Shared memory key for sales

int main() {
    int shmid_sales = shmget(SHM_KEY_SALES, sizeof(int), 0666);

    if (shmid_sales < 0) {
        perror("Shared memory access failed");
        exit(1);
    }

    int *sales = (int *)shmat(shmid_sales, NULL, 0);

    if (sales == (void *)-1) {
        perror("Shared memory attachment failed");
        exit(1);
    }

    printf("Generating Sales Report...\n");
    printf("Total items sold: %d\n", *sales);

    shmdt(sales);

    return 0;
}
