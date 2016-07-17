#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#include "process.h"

int key;
char *process_name;

void cleanup(int sig) {
    // remove the mailbox
    int mid = msgget(key, 0666);
    if (msgctl(mid, IPC_RMID, NULL) == -1) {
        perror("Failed removing message queue: ");
    }
    // remove the file
    if (remove(process_name) != 0 ) {
        perror("Failed deleting file: ");
    }

    printf("Exiting..\n");
    exit(0);
}

int main(int argc, char** argv) {
    printf("CTRL+C to exit.\n");
    if (argc < 7) {
        perror("args: process_name, key, window_size, max_delay, timeout, rate\n");
        return -1;
    }
    process_name = argv[1];
    key = atoi(argv[2]);

    // initialize basic information and write it to a file
    if (init(process_name, key, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6])) < 0) {
        return -1;
    }

    char role[MAX_NAME_LEN];
    char receiver[MAX_NAME_LEN];
    char data[MAX_SIZE];
    signal(SIGINT, cleanup);

    while(1) {
        printf("\nRole (sender/receiver): ");
        scanf("%s", role);

        if (strcmp(role, "sender") == 0) {
            // get the receiver name and the data
            printf("\nReceiver name: ");
            scanf("%s", receiver);
            printf("Data: ");
            scanf("%s", data);
            if (send_message(receiver, data) < 0) {
                printf("Failed sending data to %s\n", receiver);
            }
        } else if (strcmp(role, "receiver") == 0) {
            if (receive_message(data) >= 0) {
                printf("Message: %s\n", data);
            }
        } else {
            printf("Invalid role: %s\n", role);
        }
    }
    return 0;
}
