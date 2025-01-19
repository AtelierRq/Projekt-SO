#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define QUEUE_KEY 1234567
#define SHM_KEY 567890
#define SEM_KEY 910115

typedef struct {
    int team_id[1000]; // Kolejka kibiców (maks. 1000 kibiców)
    int front;         // Indeks początku kolejki
    int rear;          // Indeks końca kolejki
    int count;         // Liczba kibiców w kolejce
} Queue;

int main() {
    // Tworzenie kolejki komunikatów
    int msgid = msgget(QUEUE_KEY, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("Blad tworzenia kolejki komunikatow");
        exit(1);
    }
    printf("Inicjator: Kolejka komunikatow utworzona. ID: %d\n", msgid);

    // Tworzenie pamięci współdzielonej dla kolejki kibiców
    int shmid = shmget(SHM_KEY, sizeof(Queue), 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("Blad tworzenia pamieci wspoldzielonej");
        exit(1);
    }
    printf("Inicjator: Pamiec wspoldzielona utworzona. ID: %d\n", shmid);

    // Inicjalizacja pamięci współdzielonej
    Queue *queue = shmat(shmid, NULL, 0);
    if (queue == (void *)-1) {
        perror("Blad dolaczania pamieci wspoldzielonej");
        exit(1);
    }
    queue->front = 0;
    queue->rear = 0;
    queue->count = 0;
    printf("Inicjator: Kolejka kibicow zainicjalizowana.\n");

    // Tworzenie semaforów dla stanowisk kontroli
    int semid = semget(SEM_KEY, 3, 0666 | IPC_CREAT);
    if (semid == -1) {
        perror("Blad tworzenia semaforow");
        exit(1);
    }

    // Ustawienie wartości początkowych semaforów (3 stanowiska, każde ma 3 miejsca)
    for (int i = 0; i < 3; i++) {
        if (semctl(semid, i, SETVAL, 3) == -1) {
            perror("Blad ustawiania wartosci semaforow");
            exit(1);
        }
    }
    printf("Inicjator: Semafory zainicjalizowane.\n");

    return 0;
}
