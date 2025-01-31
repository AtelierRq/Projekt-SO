#include "stadium.h"
#include <pthread.h> // bo używamy wątków

// Globalne (dla sygnałów):
static int msqid = -1, shmid = -1, semid = -1;
static shared_data* shm_ptr = NULL;

// Funkcja sprzątająca zasoby (wywoływana przy ewakuacji)
void cleanup() {
    printf("[WORKER] Usuwam IPC.\n");
    if (msqid != -1) msgctl(msqid, IPC_RMID, NULL);
    if (shmid != -1) shmctl(shmid, IPC_RMID, NULL);
    if (semid != -1) semctl(semid, 0, IPC_RMID);
}

// Handler ewakuacji => SIGINT
void sigint_handler(int sig) {
    printf("[WORKER] SIGINT => Ewakuacja.\n");
    semWait(semid, 0);
    shm_ptr->stadium_open = 0; // sygnał do fanów
    semSignal(semid, 0);

    // czekamy aż current_inside=0
    while (1) {
        semWait(semid, 0);
        int inside = shm_ptr->current_inside;
        semSignal(semid, 0);

        if (inside == 0) {
            printf("[WORKER] Stadion pusty, koncze.\n");
            cleanup();
            _exit(0);
        }
        sleep(1);
    }
}

// Handler STOP => SIGUSR1
void sigusr1_handler(int sig) {
    printf("[WORKER] Odebrano STOP => allow_entrance=0\n");
    semWait(semid, 0);
    shm_ptr->allow_entrance = 0;
    semSignal(semid, 0);
}

// Handler RESUME => SIGUSR2
void sigusr2_handler(int sig) {
    printf("[WORKER] Odebrano RESUME => allow_entrance=1\n");
    semWait(semid, 0);
    shm_ptr->allow_entrance = 1;
    semSignal(semid, 0);
}

// Pomocnicze funkcje do FIFO w kolejce stacji
static void queue_skip_fan(shared_data* s, int station) {
    int front = s->queue_front[station];
    fan_info fi = s->station_queue[station][front];
    s->queue_front[station] = (s->queue_front[station] + 1) % QUEUE_SIZE;
    s->station_queue[station][ s->queue_rear[station] ] = fi;
    s->queue_rear[station] = (s->queue_rear[station] + 1) % QUEUE_SIZE;
}

static fan_info queue_front_fan(shared_data* s, int station) {
    return s->station_queue[station][ s->queue_front[station] ];
}

static fan_info queue_pop_front(shared_data* s, int station) {
    fan_info fi = s->station_queue[station][ s->queue_front[station] ];
    s->queue_front[station] = (s->queue_front[station] + 1) % QUEUE_SIZE;
    s->queue_count[station]--;
    return fi;
}

// ----------------------------------------------
// Struktura do przekazania parametru wątkowi "kontrola"
typedef struct {
    int stationIdx;
    int pidFan;
} control_param;

// Funkcja wątku "kontrola" – zamiast lambdy
void* control_thread(void* arg) {
    control_param* p = (control_param*)arg;
    int stationIdx = p->stationIdx;
    int pidFan     = p->pidFan;
    free(p);

    // Symulacja kontroli (4 sekundy)
    printf("[STATION %d] Kontroluje fana pid=%d przez 4 sek.\n", stationIdx, pidFan);
    sleep(4);

    // Po kontroli => station_in_use--
    semWait(semid, 0);
    shm_ptr->station_in_use[stationIdx]--;
    int left = shm_ptr->station_in_use[stationIdx];
    printf("[STATION %d] Fan pid=%d ukonczyl kontrole, inUse=%d.\n",
           stationIdx, pidFan, left);

    // Jeśli stacja się opróżni => station_team[stationIdx] = -1
    if (left == 0) {
        shm_ptr->station_team[stationIdx] = -1;
    }
    semSignal(semid, 0);

    return NULL;
}

// ----------------------------------------------
// Wątek obsługi danej stacji
void* station_thread(void* arg) {
    int i = *(int*)arg; // index stacji

    while (1) {
        semWait(semid, 0);

        // jeśli stadium_open=0 => ewakuacja => wątek kończy się
        if (shm_ptr->stadium_open == 0) {
            semSignal(semid, 0);
            pthread_exit(NULL);
        }

        int canAdd = 1;
        if (shm_ptr->allow_entrance == 0) {
            canAdd = 0;
        }
        if (shm_ptr->current_inside >= MAX_STADIUM_CAP) {
            canAdd = 0;
        }
        if (shm_ptr->queue_count[i] <= 0) {
            // brak kibiców w kolejce
            semSignal(semid, 0);
            usleep(200000);
            continue;
        }
        if (!canAdd) {
            // np. przerwij i czekaj
            semSignal(semid, 0);
            usleep(200000);
            continue;
        }

        int stTeam = shm_ptr->station_team[i];
        int stUse  = shm_ptr->station_in_use[i];

        // Fan z czoła
        fan_info f = queue_front_fan(shm_ptr, i);

        // Próbujemy przepuścić do 5 kibiców, jeśli drużyna się nie zgadza
        int skipCount = 0;
        while (stTeam != -1 && f.team != stTeam && skipCount < 5) {
            queue_skip_fan(shm_ptr, i);
            skipCount++;
            f = queue_front_fan(shm_ptr, i);
        }

        // jeśli stacja jest zajęta inną drużyną i nadal się nie zgadza – czekamy
        if (stTeam != -1 && f.team != stTeam) {
            semSignal(semid, 0);
            usleep(200000);
            continue;
        }

        // jeśli stacja pusta => przypisujemy drużynę
        if (stTeam == -1) {
            stTeam = f.team;
            shm_ptr->station_team[i] = stTeam;
        }

        // sprawdzamy, czy jest miejsce
        if (stUse >= STATION_CAPACITY) {
            semSignal(semid, 0);
            usleep(200000);
            continue;
        }

        // Mamy miejsce => fan wchodzi
        fan_info fanPop = queue_pop_front(shm_ptr, i);

        shm_ptr->current_inside++;      // fan "w środku" (choć tak naprawdę czeka w stacji)
        shm_ptr->station_in_use[i]++;

        printf("[WORKER] Stacja %d: wchodzi fan pid=%d (team=%d), inUse=%d\n",
               i, fanPop.pid, fanPop.team, shm_ptr->station_in_use[i]);

        // Przygotowanie parametrów do wątku "kontrola"
        control_param* cp = malloc(sizeof(control_param));
        cp->stationIdx = i;
        cp->pidFan     = fanPop.pid;

        semSignal(semid, 0);

        // Tworzymy wątek "kontrola"
        pthread_t tid;
        pthread_create(&tid, NULL, control_thread, cp);
        pthread_detach(tid);

        usleep(200000); // drobna pauza, by nie mielić w kółko
    }
}

int main() {
    // Uzyskanie dostępu do zasobów IPC
    msqid = msgget(BASE_IPC_KEY, 0666);
    if (msqid == -1) {
        perror("msgget");
        return 1;
    }
    shmid = shmget(BASE_IPC_KEY, sizeof(shared_data), 0666);
    if (shmid == -1) {
        perror("shmget");
        return 1;
    }
    semid = semget(BASE_IPC_KEY, 1, 0666);
    if (semid == -1) {
        perror("semget");
        return 1;
    }

    shm_ptr = (shared_data*) shmat(shmid, NULL, 0);
    if ((void*)shm_ptr == (void*)-1) {
        perror("shmat");
        return 1;
    }

    printf("[WORKER] Start: msqid=%d, shmid=%d, semid=%d\n",
           msqid, shmid, semid);

    // Ustaw handlery sygnałów
    signal(SIGINT,  sigint_handler);  // ewakuacja
    signal(SIGUSR1, sigusr1_handler); // STOP
    signal(SIGUSR2, sigusr2_handler); // RESUME

    // Uruchamiamy 3 wątki stacji
    pthread_t st_threads[NUM_STATIONS];
    int st_index[NUM_STATIONS];
    for (int i = 0; i < NUM_STATIONS; i++) {
        st_index[i] = i;
        pthread_create(&st_threads[i], NULL, station_thread, &st_index[i]);
    }

    // Odbieranie komunikatów => dopisywanie kibica do kolejki
    while (1) {
        message_t msg;
        if (msgrcv(msqid, &msg, sizeof(msg.mtext), 1, 0) == -1) {
            if (errno == EINTR) {
                continue; // przerwane przez sygnał
            }
            perror("[WORKER] msgrcv");
            break;
        }

        // Parsujemy: "team age station pid"
        int team, age, st, fanPid;
        if (sscanf(msg.mtext, "%d %d %d %d", &team, &age, &st, &fanPid) < 4) {
            fprintf(stderr, "[WORKER] Bledny format: %s\n", msg.mtext);
            continue;
        }

        semWait(semid, 0);

        if (shm_ptr->allow_entrance == 1
            && shm_ptr->stadium_open == 1
            && shm_ptr->current_inside < MAX_STADIUM_CAP
            && shm_ptr->queue_count[st] < QUEUE_SIZE)
        {
            int r = shm_ptr->queue_rear[st];
            fan_info fi;
            fi.team = team;
            fi.age  = age;
            fi.pid  = fanPid;  // PID fana (z komunikatu)

            shm_ptr->station_queue[st][r] = fi;
            shm_ptr->queue_rear[st] = (r + 1) % QUEUE_SIZE;
            shm_ptr->queue_count[st]++;

            printf("[WORKER] Fan(team=%d, age=%d, pid=%d) => stacja %d => queue_count=%d\n",
                   team, age, fanPid, st, shm_ptr->queue_count[st]);
        } else {
            // Odrzucamy kibica
            printf("[WORKER] Odrzucono kibica (team=%d, pid=%d) na st=%d\n",
                   team, fanPid, st);
        }

        semSignal(semid, 0);
    }

    // Koniec pętli msgrcv
    cleanup();
    return 0;
}
