#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <time.h>

#define QUEUE_KEY 1234567

typedef struct {
    long type;   // Typ wiadomości
    int command; // Polecenie od pracownika technicznego (2 = wpuszczanie, 3 = opuszczanie)
    int team_id; // ID drużyny kibica
} Message;

int main() {
    int msgid = msgget(QUEUE_KEY, 0666);
    if (msgid == -1) {
        perror("Blad otwierania kolejki komunikatow");
        exit(1);
    }

    srand(getpid());
    int my_team = rand() % 2 + 1;

    // Wyślij wiadomość o wejściu do kolejki
    Message msg;
    msg.type = 2;            // Typ wiadomości dla pracownika
    msg.command = 2;         // Próba wejścia
    msg.team_id = my_team;

    if (msgsnd(msgid, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
        perror("Blad wysylania wiadomosci do pracownika");
        exit(1);
    }

    printf("Kibic z drużyny %d czeka na wejście na stadion.\n", my_team);

    // Oczekiwanie na odpowiedź od pracownika
    if (msgrcv(msgid, &msg, sizeof(msg) - sizeof(long), 2, 0) == -1) {
        perror("Blad odbioru wiadomosci od pracownika");
        exit(1);
    }

    if (msg.command == 2) {
        printf("Kibic z drużyny %d wchodzi na stadion.\n", my_team);
    } else if (msg.command == 3) {
        printf("Kibic z drużyny %d opuszcza stadion.\n", my_team);
    }

    return 0;
}
