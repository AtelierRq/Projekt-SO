#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define QUEUE_KEY 947270
#define SHM_KEY 24125
#define SEM_KEY 91011

int main() {
    // Tworzenie kolejki komunikatow
    int msgid = msgget(QUEUE_KEY, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("Blad tworzenia kolejki komunikatow");
        exit(1);
    }
    printf("Inicjator: Kolejka komunikatow utworzona. ID: %d\n", msgid);

    // Tworzenie pamieci wspoldzielonej dla stanowisk
    int shmid = shmget(SHM_KEY, 3 * sizeof(int), 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("Blad tworzenia pamieci wspoldzielonej");
        exit(1);
    }
    printf("Inicjator: Pamiec wspoldzielona utworzona. ID: %d\n", shmid);

    // Tworzenie semaforow dla stanowisk
    int semid = semget(SEM_KEY, 3, 0666 | IPC_CREAT);
    if (semid == -1) {
        perror("Blad tworzenia semaforow");
        exit(1);
    }
    for (int i = 0; i < 3; i++) {
        semctl(semid, i, SETVAL, 3); // Na kazdym stanowisku max 3 osoby
    }
    printf("Inicjator: Semafory utworzone.\n");

    return 0;
}
