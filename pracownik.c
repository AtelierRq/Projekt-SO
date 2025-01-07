#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#define QUEUE_KEY 947270

typedef struct {
    long type;
    int command;
} Message;

void handle_signal(int msgid, int command) {
    Message msg;
    msg.type = 2; // Pracownik przekazuje wiadomosci kibicom

    if (command == 1) {
        sleep(0.5);
        printf("Pracownik techniczny: Wstrzymuje wpuszczanie kibicow.\n");
    } else if (command == 2) {
        sleep(0.5);
        printf("Pracownik techniczny: Wznawiam wpuszczanie kibicow.\n");
        msg.command = command;
        msgsnd(msgid, &msg, sizeof(msg.command), 0); // Informuje kibicow
    } else if (command == 3) {
        sleep(0.5);
        printf("Pracownik techniczny: Kibice opuszczaja stadion.\n");
        msg.command = command;
        msgsnd(msgid, &msg, sizeof(msg.command), 0); // Informuje kibicow
    }
}

int main() {
    int msgid = msgget(QUEUE_KEY, 0666);
    if (msgid == -1) {
        perror("Blad otwierania kolejki komunikatow");
        exit(1);
    }

    Message msg;
    while (1) {
        // Odbieranie sygnalu od kierownika
        if (msgrcv(msgid, &msg, sizeof(msg.command), 1, 0) == -1) {
            perror("Blad odbioru wiadomosci w pracowniku");
            exit(1);
        }
        handle_signal(msgid, msg.command);

        // Zakonczenie dzialania po sygnale 3
        if (msg.command == 3) {
            sleep(1);
            printf("Pracownik techniczny: Przesylam informacje do kierownika.\n");
            break;
        }
    }

    return 0;
}
