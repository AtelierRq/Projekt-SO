#include "stadium.h"

int main() {
    // Uzyskanie dostępu do IPC
    int msqid = msgget(BASE_IPC_KEY, 0666);
    if (msqid == -1) {
        perror("[FAN] msgget");
        return 1;
    }
    int shmid = shmget(BASE_IPC_KEY, sizeof(shared_data), 0666);
    if (shmid == -1) {
        perror("[FAN] shmget");
        return 1;
    }
    int semid = semget(BASE_IPC_KEY, 1, 0666);
    if (semid == -1) {
        perror("[FAN] semget");
        return 1;
    }

    shared_data* shm_ptr = (shared_data*) shmat(shmid, NULL, 0);
    if ((void*)shm_ptr == (void*)-1) {
        perror("[FAN] shmat");
        return 1;
    }

    srand(time(NULL) ^ getpid());
    int myTeam   = rand() % 2;      // 0 lub 1
    int myAge    = 18 + (rand() % 30);
    int station  = rand() % NUM_STATIONS; // losowa stacja (0..2)
    int myPid    = getpid();        // ID fana

    printf("[FAN %d] Przybywam do stacji=%d, team=%d, age=%d\n",
           myPid, station, myTeam, myAge);

    // Wysyłamy komunikat (mtype=1) do worker-a
    // Format: "team age station pid"
    message_t req;
    req.mtype = 1;
    snprintf(req.mtext, sizeof(req.mtext), "%d %d %d %d",
             myTeam, myAge, station, myPid);

    if (msgsnd(msqid, &req, strlen(req.mtext)+1, 0) == -1) {
        perror("[FAN] msgsnd");
        shmdt(shm_ptr);
        return 1;
    }

    // Fan czeka w pętli, dopóki "stadium_open" = 1
    while (1) {
        semWait(semid, 0);
        int open = shm_ptr->stadium_open;
        semSignal(semid, 0);

         if (open == 0) {
            // => ewakuacja
            semWait(semid, 0);
            shm_ptr->current_inside--;
            semSignal(semid, 0);

            // koniec procesu
            printf("[FAN %d] Ewakuacja -> wychodzę.\n", getpid());
            break;
        }
    }

    // Kibic wychodzi dopiero przy ewakuacji
    printf("[FAN %d] Ewakuacja -> koncze.\n", myPid);

    shmdt(shm_ptr);
    return 0;
}
