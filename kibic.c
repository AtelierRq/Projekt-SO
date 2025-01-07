#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define QUEUE_KEY 947270

typedef struct {
    long type;
    int command;
} Message;

int main() {
    int msgid = msgget(QUEUE_KEY, 0666);
    if (msgid == -1) {
        perror("Blad otwierania kolejki komunikatow");
        exit(1);
    }

    Message msg;
    while (1) {
        // Odbieranie sygnalu od pracownika (type = 2)
        if (msgrcv(msgid, &msg, sizeof(msg.command), 2, 0) == -1) {
            perror("Blad odbioru wiadomosci w kibicu");
            exit(1);
        }

        if (msg.command == 2) {
            printf("Kibic: Wchodze na stadion.\n");
        } else if (msg.command == 3) {
            printf("Kibic: Opuszczam stadion.\n");
            break; // Kibic konczy dzialanie po sygnale 3
        }
    }

    return 0;
}
