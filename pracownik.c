#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>

#define QUEUE_KEY 947270
#define SHM_KEY 5678
#define SEM_KEY 91011
#define MAX_K 10000 // Maksymalna liczba kibicow na stadionie

typedef struct {
    long type;
    int command;   // 2 = wejscie, 3 = opuszczenie stadionu
    int team_id;   // ID druzyny kibica
} Message;

int current_kibice = 0; // Liczba kibicow aktualnie na staddionie

void zajmij_semafor(int semid, int index) {
    struct sembuf operacja = {index, -1, 0}; // Zmniejsz wartosc semafora o 1
    semop(semid, &operacja, 1);
}

void zwolnij_semafor(int semid, int index) {
    struct sembuf operacja = {index, 1, 0}; // Zwieksz wartosc semafora o 1
    semop(semid, &operacja, 1);
}

int main() {
    // Uzyskanie dostepow do kolejki, pamieci wspoldzielonej i semaforow
    int msgid = msgget(QUEUE_KEY, 0666);
    if (msgid == -1) {
        perror("Blad otwierania kolejki komunikatow");
        exit(1);
    }

    int shmid = shmget(SHM_KEY, 3 * sizeof(int), 0666);
    if (shmid == -1) {
        perror("Blad otwierania pamieci wspoldzielonej");
        exit(1);
    }

    int *stanowiska = shmat(shmid, NULL, 0);
    if (stanowiska == (void *)-1) {
        perror("Blad dolaczania pamieci wspoldzielonej");
        exit(1);
    }

    int semid = semget(SEM_KEY, 3, 0666);
    if (semid == -1) {
        perror("Blad otwierania semaforow");
        exit(1);
    }
    
    Message msg;
    while (1) {
        // Odbieranie wiadomosci od kibicow
        if (msgrcv(msgid, &msg, sizeof(msg.command) + sizeof(msg.team_id), 1, 0) == -1) {
            perror("Blad odbioru wiadomosci");
            exit(1);
        }

        if (msg.command == 2) { // Kibic chce wejsc
            if (current_kibice >= MAX_K) {
                printf("Pracownik: Limit kibicow na stadionie osiagniety (%d/%d). Kibic z druzyny %d nie moze wej>
                       current_kibice, MAX_K, msg.team_id);
                continue; // Nie wpuszczamy kibica
            }

            int znaleziono_stanowisko = 0;

            // Szukanie wolnego stanowiska
            for (int i = 0; i < 3; i++) {
                if (stanowiska[i] == 0 || stanowiska[i] == msg.team_id) {
                    zajmij_semafor(semid, i); // Zajmij miejsce
                    stanowiska[i] = msg.team_id; // Przypisz druzyne do stanowiska
                    current_kibice++;
                    printf("Pracownik: Kibic z druzyny %d zajmuje stanowisko %d. Liczba kibicow: %d/%d\n",
                           msg.team_id, i + 1, current_kibice, MAX_K);
                    znaleziono_stanowisko = 1;
                    break;
                }
            }

            if (!znaleziono_stanowisko) {
                printf("Pracownik: Brak stanowiska dla kibica z druzyny %d.\n", msg.team_id);
            }
        } else if (msg.command == 3) { // Kibic opuszcza stadion
            for (int i = 0; i < 3; i++) {
                if (stanowiska[i] == msg.team_id) {
                    zwolnij_semafor(semid, i); // Zwolnij miejsce
                    stanowiska[i] = 0; // Wyczysc druzyne ze stanowiska
                    current_kibice--;
                    printf("Pracownik: Kibic z druzyny %d opuszcza stanowisko %d. Liczba kibicow: %d/%d\n",
                           msg.team_id, i + 1, current_kibice, MAX_K);
                    break;
                }
            }
        }
    }

    return 0;
}

