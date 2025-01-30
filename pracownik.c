include "stadium.h"
#include <pthread.h>  // jeśli byłbyś wielowątkowy, ale tu multi-proc

static int msqid=-1, shmid=-1, semid=-1;
static shared_data* shm_ptr = NULL;

// Flagi do obsługi bezpiecznego wstrzymania
static volatile sig_atomic_t g_preparing_suspend = 0; // 1=gdy zaraz wstrzymujemy
static volatile sig_atomic_t g_paused            = 0; // nie zawsze potrzebne

// Cleanup
void cleanup() {
    printf("[WORKER] cleanup: usuwam kolejke, pamiec i semafor.\n");
    if (msqid != -1) msgctl(msqid, IPC_RMID, NULL);
    if (shmid != -1) shmctl(shmid, IPC_RMID, NULL);
    if (semid != -1) semctl(semid, 0, IPC_RMID);
}

// Handler SIGINT => ewakuacja
void sigint_handler(int sig) {
    printf("[WORKER] Odebrano SIGINT => ewakuacja.\n");
    semWait(semid, 0);
    shm_ptr->stadium_open = 0;
    semSignal(semid, 0);

    // czekamy aż current_inside == 0
    while (1) {
        semWait(semid, 0);
        int inside = shm_ptr->current_inside;
        semSignal(semid, 0);

        if (inside == 0) {
            printf("[WORKER] Stadion pusty. Koncze prace.\n");
            cleanup();
            _exit(0);
        }
        sleep(1);
    }
}

// Handler SIGUSR1 => STOP wejść
void sigusr1_handler(int sig) {
    printf("[WORKER] Odebrano SIGUSR1 => STOP.\n");
    semWait(semid, 0);
    shm_ptr->allow_entrance = 0;
    semSignal(semid, 0);
}

// Handler SIGUSR2 => RESUME wejść
void sigusr2_handler(int sig) {
    printf("[WORKER] Odebrano SIGUSR2 => RESUME.\n");
    semWait(semid, 0);
    shm_ptr->allow_entrance = 1;
    semSignal(semid, 0);
}

// -------------- NOWE: SIGTSTP / SIGCONT -----------------

// Handler SIGTSTP => przygotowanie do wstrzymania
void sigtstp_handler(int sig) {
    fprintf(stderr, "\n[WORKER] Odebrano SIGTSTP (Ctrl+Z). Przygotowuje sie do pauzy...\n");

    // ustawiamy flage
    g_preparing_suspend = 1;

    // Wejdź w sekcję krytyczną, żeby mieć pewność, że nikt nie jest w trakcie
    semWait(semid, 0);
    // W tym momencie mamy „muteks” – nikt nie modyfikuje stadium
    semSignal(semid, 0);

    fprintf(stderr, "[WORKER] Bezpiecznie wychodze z operacji. Wstrzymuje proces.\n");

    // Przywróć domyślną obsługę SIGTSTP i wywołaj
    signal(SIGTSTP, SIG_DFL);
    raise(SIGTSTP);
}

// Handler SIGCONT => wznawiamy
void sigcont_handler(int sig) {
    fprintf(stderr, "[WORKER] Odebrano SIGCONT => wznawiam prace!\n");

    g_preparing_suspend = 0;
    g_paused = 0; // jeśli byśmy używali

    // ewentualnie cokolwiek do ponownej inicjalizacji
}

// ---------------------------------------------------------

int main() {
    msqid = msgget(BASE_IPC_KEY, 0666);
    if (msqid == -1) {
        perror("msgget");
        exit(1);
    }
    shmid = shmget(BASE_IPC_KEY, sizeof(shared_data), 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }
    semid = semget(BASE_IPC_KEY, 1, 0666);
    if (semid == -1) {
        perror("semget");
        exit(1);
    }
    shm_ptr = (shared_data*) shmat(shmid, NULL, 0);
    if ((void*)shm_ptr == (void*)-1) {
        perror("shmat");
        exit(1);
    }

    printf("[WORKER] Start (msqid=%d, shmid=%d, semid=%d)\n", msqid, shmid, semid);

    // Ustaw handlery
    signal(SIGINT,  sigint_handler);
    signal(SIGUSR1, sigusr1_handler);
    signal(SIGUSR2, sigusr2_handler);
    signal(SIGTSTP, sigtstp_handler);
    signal(SIGCONT, sigcont_handler);

    // Pętla główna odbierania komunikatów od fanów
    while (1) {
        // Jeśli przed chwilą zaczął się proces wstrzymywania, dajmy mu szansę
        // (opcjonalnie)
        if (g_preparing_suspend) {
            // czekamy troche, aż system wykona raise(SIGTSTP)
            usleep(100000);
            continue;
        }

        message_t request;
        if (msgrcv(msqid, &request, sizeof(request.mtext), 1, 0) == -1) {
            if (errno == EINTR) {
                // przerwane przez sygnał – kontynuuj
                continue;
            }
            perror("[WORKER] msgrcv error");
            break;
        }

        int fanTeam, fanAge;
        sscanf(request.mtext, "%d %d", &fanTeam, &fanAge);

        message_t response;
        response.mtype = 2;

        semWait(semid, 0);

        int canEnter = 1;
        // czy w ogóle wpuszczamy?
        if (shm_ptr->allow_entrance == 0 || shm_ptr->stadium_open == 0) {
            canEnter = 0;
        }
        // limit 1000
        if (canEnter && shm_ptr->current_inside >= MAX_STADIUM_CAP) {
            canEnter = 0;
        }

        // stacje
        int stationIndex = -1;
        if (canEnter) {
            for (int i=0; i<NUM_STATIONS; i++) {
                if ((shm_ptr->station_team[i] == -1 || shm_ptr->station_team[i] == fanTeam)
                     && shm_ptr->station_in_use[i] < STATION_CAPACITY)
                {
                    stationIndex = i;
                    break;
                }
            }
            if (stationIndex == -1) {
                canEnter = 0;
            }
        }

        if (canEnter) {
            if (shm_ptr->station_team[stationIndex] == -1) {
                shm_ptr->station_team[stationIndex] = fanTeam;
            }
            shm_ptr->station_in_use[stationIndex]++;
            shm_ptr->current_inside++;
            strcpy(response.mtext, "OK");
            printf("[WORKER] Fan(team=%d,age=%d) => stacja=%d, inside=%d\n",
                fanTeam, fanAge, stationIndex, shm_ptr->current_inside);
        } else {
            strcpy(response.mtext, "NO");
            //printf("[WORKER] Odmowa fan(team=%d)\n", fanTeam);
        }

        semSignal(semid, 0);

        if (msgsnd(msqid, &response, strlen(response.mtext)+1, 0) == -1) {
            perror("[WORKER] msgsnd error");
        }
    }

    cleanup();
    return 0;
}
