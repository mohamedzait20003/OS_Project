#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

void print_menu() {
    printf("\nPlease select an option:\n");
    printf("1. Terminate Server (SIGTERM)\n");
    printf("2. Stop Server (SIGSTOP)\n");
    printf("3. Continue Server (SIGCONT)\n");
    printf("4. Kill Server (SIGKILL)\n");
    printf("5. Exit\n");
    printf("Enter your choice: ");
}

int send_signal(pid_t pid, int signal) {
    if (kill(pid, signal) == 0) {
        printf("Signal %d sent to server with PID %d.\n", signal, pid);
        return 0;
    } else {
        perror("Failed to send signal");
        return errno;
    }
}

int update_server_pids(pid_t *server_pids, int max_pids) {
    FILE *fp;
    char path[1035];
    int pid_count = 0;

    // Run the pgrep command to get server PIDs
    fp = popen("pgrep -f \"./server\"", "r");
    if (fp == NULL) {
        printf("Failed to run pgrep command\n");
        return -1;
    }

    // Read the output of the pgrep command
    while (fgets(path, sizeof(path) - 1, fp) != NULL && pid_count < max_pids) {
        server_pids[pid_count++] = (pid_t)atoi(path);
    }

    // Close the pgrep command
    pclose(fp);

    return pid_count;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <server_pid1> [<server_pid2> ...]\n", argv[0]);
        return 1;
    }

    int pid_count = argc - 1;
    pid_t server_pids[pid_count - 1];

    for (int i = 1; i < argc; i++) {
        server_pids[i - 1] = (pid_t)atoi(argv[i]);
    }

    while(1){
        print_menu();

        int choice;
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                for (int i = 0; i < argc - 1; i++) {
                    if (send_signal(server_pids[i], SIGTERM) == ESRCH) {
                        pid_count = update_server_pids(server_pids, 100);
                    }
                }
                break;
            case 2:
                for (int i = 0; i < argc - 1; i++) {
                    if (send_signal(server_pids[i], SIGSTOP) == ESRCH) {
                        pid_count = update_server_pids(server_pids, 100);
                    }
                }
                break;
            case 3:
                for (int i = 0; i < argc - 1; i++) {
                    if (send_signal(server_pids[i], SIGCONT) == ESRCH) {
                        pid_count = update_server_pids(server_pids, 100);
                    }
                }
                break;
            case 4:
                for (int i = 0; i < argc - 1; i++) {
                    if (send_signal(server_pids[i], SIGKILL) == ESRCH) {
                        pid_count = update_server_pids(server_pids, 100);
                    }
                }
                break;
            case 5:
                printf("Exiting...\n");
                return 0;
            default:
                printf("Invalid option. Please try again.\n");
                break;
        }
    }

    return 0;
}
