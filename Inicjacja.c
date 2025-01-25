#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define QUEUE_KEY 947270

int main() {
    int msgid = msgget(QUEUE_KEY, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("msgget");
        exit(1);
    }

    printf("Inicjator: Kolejka komunikatow utworzona. ID: %d\n", msgid);
    return 0;
}

