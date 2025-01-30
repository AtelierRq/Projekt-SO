#include "stadium.h"

int main() {
    // 1. Tworzymy kolejkę
    int msqid = msgget(BASE_IPC_KEY, IPC_CREAT | 0666);
    if (msqid == -1) {
        perror("msgget");
        exit(1);
    }
    printf("[INIT] Kolejka utworzona (msqid=%d)\n", msqid);

    // 2. Pamięć współdzielona
    int shmid = shmget(BASE_IPC_KEY, sizeof(shared_data), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }
    printf("[INIT] Pamiec wspoldzielona (shmid=%d)\n", shmid);

    shared_data* shm_ptr = (shared_data*) shmat(shmid, NULL, 0);
    if ((void*)shm_ptr == (void*)-1) {
        perror("shmat");
        exit(1);
    }

    // 3. Semafor (1 sztuka)
    int semid = semget(BASE_IPC_KEY, 1, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget");
        exit(1);
    }
    union semun arg;
    arg.val = 1;
    if (semctl(semid, 0, SETVAL, arg) == -1) {
        perror("semctl");
        exit(1);
    }
    printf("[INIT] Semafor=%d utworzony i ustawiony\n", semid);

    // 4. Inicjalizacja struct w pamięci
    semWait(semid, 0);

    shm_ptr->stadium_open   = 1;
    shm_ptr->allow_entrance = 1;
    shm_ptr->current_inside = 0;
    for (int i = 0; i < NUM_STATIONS; i++) {
        shm_ptr->station_in_use[i] = 0;
        shm_ptr->station_team[i]   = -1;
    }

    semSignal(semid, 0);

    printf("[INIT] Struktura IPC zainicjalizowana.\n");
    printf("[INIT] Czekam w pause(), aby nie zwolnic zasobow...\n");

    pause(); // czekaj (ew. Ctrl+C) – zasoby pozostają w systemie
    shmdt(shm_ptr);
    return 0;
}
