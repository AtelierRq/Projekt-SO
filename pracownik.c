#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#define QUEUE_KEY 1234

typedef struct {
    long type;
    int command;
} Message;

void handle_signal(int signal) {
    if (signal == 1) {
        printf("Pracownik techniczny: Wstrzymuje wpuszczanie kibicow.\n");
    } else if (signal == 2) {
        printf("Pracownik techniczny: Wznawiam wpuszczanie kibicow. \n");
    } else if (signal == 3) {
        printf("Pracownik techniczny: Kibice opuszczaja stadion. \n");
    }
}

int main() {
    int msgid = msgget(QUEUE_KEY, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("msgget");
        exit(1);
    }

    Message msg;
    while (1) {
        if (msgrcv(msgid, &msg, sizeof(msg.command), 1, 0) != -1) {
            handle_signal(msg.command);
            if (msg.command == 3) break; // zakonczenie na sygnal 3
        }
    }

    printf("Pracownik techniczny: Przesylam informacje do kierownika. \n");
    return 0;
}
