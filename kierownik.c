#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#define QUEUE_KEY 947270

typedef struct {
    long type;
    int command;   // 1 = wstrzymanie, 2 = wpuszczanie, 3 = opuszczanie
    int team_id;   // ID druzyny kibica
} Message;

// Funkcja do wysylania sygnalu
void send_signal(int msgid, int command, int team_id) {
    Message msg;
    msg.type = 1;          // Typ wiadomosci dla pracownika
    msg.command = command; // Komenda
    msg.team_id = team_id; // ID druzyny (0 dla sygnalow bez druzyny)

    msgsnd(msgid, &msg, sizeof(msg.command) + sizeof(msg.team_id), 0);
    if (command == 1) {
        printf("Kierownik wysyla sygnal: Wstrzymanie wpuszczania kibicow.\n");
    } else if (command == 2) {
        printf("Kierownik wysyla sygnal: Wpuszczanie kibicow z druzyny %d.\n", team_id);
    } else if (command == 3) {
        printf("Kierownik wysyla sygnal: Opuszczanie stadionu.\n");
    }
}

int main() {
    // Uzyskanie dostepu do kolejki komunikatow
    int msgid = msgget(QUEUE_KEY, 0666);
    if (msgid == -1) {
        perror("Blad otwierania kolejki komunikatow");
        exit(1);
    }

    printf("Kierownik: Maksymalna liczba kibicow na stadionie: 10000.\n");
    printf("Kierownik: Rozpoczynam wysylanie sygnalow...\n");

    // Symulacja wysylania sygnalow
    send_signal(msgid, 1, 0); // Sygnal 1: Wstrzymanie
    sleep(3);

    send_signal(msgid, 2, 1); // Sygnal 2: Wpuszczanie druzyny 1
    sleep(3);

    send_signal(msgid, 2, 2); // Sygnal 3: Wpuszczanie druzyny 2
    sleep(3);

    send_signal(msgid, 3, 0); // Sygnal 3: Opuszczanie stadionu
    printf("Kierownik: Zakonczono wysylanie sygnalow.\n");

    return 0;
}
