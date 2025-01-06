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

void send_signal(int msgid, int signal) {
    Message msg;
    msg.type = 1;
    msg.command = signal;
    msgsnd(msgid, &msg, sizeof(msg.command), 0);
    printf("Kierownik wysyla sygnal: %d\n", signal);
}

int main() {
    int msgid = msgget(QUEUE_KEY, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("msgget");
        exit(1);
    }

    printf("Kierownik: Wysylam sygnaly...\n");
    send_signal(msgid, 1); // sygnal 1 - wstrzymanie wpuszczania
    sleep(5);

    send_signal(msgid, 2); // sygnal 2 - wznowienie wpuszczania
    sleep(5);

    send_signal(msgid, 3); // sygnal 3 - wyjscie kibicow
    sleep(5);

    printf("Kierownik: Zakonczono wysylanie sygnalow.\n");
    return 0;
}
