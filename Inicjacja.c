#include "stadium.h"

int main() {
    // 1. Kolejka komunikatów
    int msqid = msgget(BASE_IPC_KEY, IPC_CREAT | 0666);
    if (msqid == -1) {
        perror("msgget");
        return 1;
    }
    printf("[INIT] Utworzono kolejke => msqid=%d\n", msqid);

    // 2. Pamięć współdzielona
    int shmid = shmget(BASE_IPC_KEY, sizeof(shared_data), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        return 1;
    }
    printf("[INIT] Utworzono pamiec => shmid=%d\n", shmid);

    shared_data* shm_ptr = (shared_data*) shmat(shmid, NULL, 0);
    if ((void*)shm_ptr == (void*)-1) {
        perror("shmat");
        return 1;
    }

    // 3. Semafor
    int semid = semget(BASE_IPC_KEY, 1, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget");
        return 1;
    }
    union semun arg;
    arg.val = 1;  // muteks = 1
    if (semctl(semid, 0, SETVAL, arg) == -1) {
        perror("semctl");
        return 1;
    }
    printf("[INIT] Semafor=%d utworzony\n", semid);

    // 4. Inicjalizacja w memory
    semWait(semid, 0);

    shm_ptr->stadium_open   = 1;
    shm_ptr->allow_entrance = 1;
    shm_ptr->current_inside = 0;

    for (int i=0; i<NUM_STATIONS; i++) {
        shm_ptr->station_in_use[i] = 0;
        shm_ptr->station_team[i]   = -1;

        shm_ptr->queue_count[i]    = 0;
        shm_ptr->queue_front[i]    = 0;
        shm_ptr->queue_rear[i]     = 0;

        for (int j=0; j<QUEUE_SIZE; j++) {
            shm_ptr->station_queue[i][j].team = -1;
            shm_ptr->station_queue[i][j].age  = -1;
            shm_ptr->station_queue[i][j].pid  = -1;
        }
    }

    semSignal(semid, 0);

    printf("[INIT] Zainicjalizowano. Pause() aby nie zwolnic zasobow.\n");
    pause(); // czekamy, by nie usunąć zasobów
    shmdt(shm_ptr);
    return 0;
}
