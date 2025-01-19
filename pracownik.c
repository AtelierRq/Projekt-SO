#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#define QUEUE_KEY 1234567

typedef struct {
    long type;
    int command; // Komenda od kierownika (1, 2, 3)
    int team_id; // ID drużyny (opcjonalnie)
} Message;

void handle_signal(int msgid, Message *msg) {
    if (msg->command == 1) {
        printf("Pracownik: Wstrzymuje wpuszczanie kibicow.\n");
    } else if (msg->command == 2) {
        printf("Pracownik: Rozpoczyna wpuszczanie kibicow.\n");

        // Informowanie kibiców
        Message to_kibic;
        to_kibic.type = 2; // Typ wiadomości dla kibiców
        to_kibic.command = 2;
        if (msgsnd(msgid, &to_kibic, sizeof(to_kibic.command), 0) == -1) {
            perror("Blad wysylania sygnalu do kibicow");
        }
    } else if (msg->command == 3) {
        printf("Pracownik: Rozpoczyna opuszczanie stadionu.\n");

        // Informowanie kibiców
        Message to_kibic;
        to_kibic.type = 2; // Typ wiadomości dla kibiców
        to_kibic.command = 3;
        if (msgsnd(msgid, &to_kibic, sizeof(to_kibic.command), 0) == -1) {
            perror("Blad wysylania sygnalu do kibicow");
        }
    }
}

int main() {
    int msgid = msgget(QUEUE_KEY, 0666);
    if (msgid == -1) {
        perror("Blad otwierania kolejki komunikatow");
        exit(1);
    }

    printf("Pracownik: Oczekuje na sygnaly...\n");

    Message msg;
    while (1) {
        if (msgrcv(msgid, &msg, sizeof(msg.command), 1, 0) == -1) {
            perror("Blad odbioru sygnalu od kierownika");
            exit(1);
        }

        handle_signal(msgid, &msg);

        if (msg.command == 3) {
            printf("Pracownik: Kibice opuszczaja stadion.\n");
            break;
        }
    }

    return 0;
}
