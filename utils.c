#include "stadium.h"

// Proste P() i V() na semaforze (1 element = muteks)
void semWait(int semid, int semnum) {
    struct sembuf sops;
    sops.sem_num = semnum;
    sops.sem_op  = -1;  // P()
    sops.sem_flg = 0;
    semop(semid, &sops, 1);
}

void semSignal(int semid, int semnum) {
    struct sembuf sops;
    sops.sem_num = semnum;
    sops.sem_op  = 1;   // V()
    sops.sem_flg = 0;
    semop(semid, &sops, 1);
}
