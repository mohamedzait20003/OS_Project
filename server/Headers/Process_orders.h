#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define NUM_ORDERS 3       
#define SHM_KEY_INVENTORY 1234
#define SHM_KEY_SALES 5678  