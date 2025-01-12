#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#define QUEUE_KEY 947270

typedef struct {
    long type;
    int command;  // 2 = wejscie, 3 = opuszczenie stadionu
    int team_id;  // ID druzyny kibica
} Message;

int main() {
    int msgid = msgget(QUEUE_KEY, 0666);
    if (msgid == -1) {
        perror("Blad otwierania kolejki komunikatow");
        exit(1);
    }

    int my_team = rand() % 2 + 1; // Losowy numer druzyny
    printf("Kibic z druzyny %d czeka na wejscie na stadion.\n", my_team);

    Message msg;
    msg.type = 1;
    msg.command = 2; // Proba wejscia
    msg.team_id = my_team;

    // Wyslanie proby wejscia
    msgsnd(msgid, &msg, sizeof(msg.command) + sizeof(msg.team_id), 0);

    // Oczekiwanie na odpowiedz
    if (msgrcv(msgid, &msg, sizeof(msg.command) + sizeof(msg.team_id), 2, 0) == -1) {
        perror("Blad odbioru wiadomosci");
        exit(1);
    }

    if (msg.command == 2) {
        printf("Kibic z druzyny %d wchodzi na stadion.\n", my_team);
    } else if (msg.command == 3) {
        printf("Kibic z druzyny %d opuszcza stadion.\n", my_team);
    }

    return 0;
}
