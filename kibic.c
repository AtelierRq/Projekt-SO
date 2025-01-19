#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#define QUEUE_KEY 1234567

typedef struct {
    long type;
    int command; // Polecenie od pracownika (2, 3)
} Message;

int main() {
    int msgid = msgget(QUEUE_KEY, 0666);
    if (msgid == -1) {
        perror("Blad otwierania kolejki komunikatow");
        exit(1);
    }

    printf("Kibic czeka na instrukcje...\n");

    Message msg;
    if (msgrcv(msgid, &msg, sizeof(msg.command), 2, 0) == -1) {
        perror("Blad odbioru wiadomosci od pracownika");
        exit(1);
    }

    if (msg.command == 2) {
        printf("Kibic wchodzi na stadion.\n");
    } else if (msg.command == 3) {
        printf("Kibic opuszcza stadion.\n");
    }

    return 0;
}
