#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#define QUEUE_KEY 1234567

typedef struct {
    long type;
    int command; // 1 = wstrzymanie, 2 = wpuszczanie, 3 = opuszczanie
} Message;

void send_signal(int msgid, int command) {
    Message msg;
    msg.type = 1; // Typ wiadomości dla pracownika
    msg.command = command;

    if (msgsnd(msgid, &msg, sizeof(msg.command), 0) == -1) {
        perror("Blad wysylania sygnalu");
        exit(1);
    }

    if (command == 1) {
        printf("Kierownik wysyla sygnal: Wstrzymanie wpuszczania kibicow.\n");
    } else if (command == 2) {
        printf("Kierownik wysyla sygnal: Wpuszczanie kibicow.\n");
    } else if (command == 3) {
        printf("Kierownik wysyla sygnal: Opuszczanie stadionu.\n");
    }
}

int main() {
    int msgid = msgget(QUEUE_KEY, 0666);
    if (msgid == -1) {
        perror("Blad otwierania kolejki komunikatow");
        exit(1);
    }

    printf("Kierownik: Rozpoczynam wysylanie sygnalow...\n");

    send_signal(msgid, 2); // Sygnał wpuszczania
    sleep(7);

    send_signal(msgid, 1); // Sygnał wstrzymania
    sleep(7);

    send_signal(msgid, 3); // Sygnał opuszczania
    sleep(7);
    printf("Kierownik: Zakonczono wysylanie sygnalow.\n");

    return 0;
}
