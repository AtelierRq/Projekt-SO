#include "stadium.h"

/** 
 * Zasoby IPC i zmienne globalne
 */
static int msqid = -1, shmid = -1, semid = -1;
static shared_data* shm_ptr = NULL;

// Czy fan jest aktualnie "w środku" stadionu?
static int g_inStadium = 0;

// Zmienna do ewentualnego pamiętania stacji (jeśli worker zwróci nam index –
// w tym przykładzie uproszczone, bo worker sam wybiera stację. Ale jeżeli
// chcesz mieć 100% pewności, który index zajął fan, można to przekazywać w mtext).
//static int g_stationIndex = -1;

// Flagi sygnału
static volatile sig_atomic_t g_preparing_suspend = 0;
static volatile sig_atomic_t g_paused = 0;

// ----------------------------------
// Funkcje pomocnicze
// ----------------------------------
static void lock()  { 
    struct sembuf s = {0, -1, 0}; 
    semop(semid, &s, 1);
}

static void unlock() {
    struct sembuf s = {0,  1, 0};
    semop(semid, &s, 1);
}

/**
 * Handler SIGTSTP – przygotuj się do zatrzymania (Ctrl+Z).
 * 1. Ustawiamy flagę, żeby pętla główna wstrzymała aktywność.
 * 2. Jeśli akurat mamy sekcję krytyczną, wychodzimy z niej.
 * 3. Przywracamy default i wywołujemy raise(SIGTSTP).
 */
void sigtstp_handler(int signo) {
    fprintf(stderr, "[FAN %d] Otrzymano SIGTSTP => przygotowanie do wstrzymania...\n", getpid());
    g_preparing_suspend = 1; 
    g_paused = 1;   // Fan będzie w stanie 'paused' aż do SIGCONT

    // Jeśli właśnie jesteśmy w sekcji krytycznej, upewnijmy się, że ją opuszczamy.
    // W najprostszym wariancie: nic nie robimy, o ile sekcje są krótkie.
    // Ale jeśli fan trzyma lock => zwolnijmy go:
    // (W tym kodzie fan raczej nie trzyma locka na dłużej – chyba że w pętli czeka.)
    // Możesz dodać logikę np.:
    // lock();   // by upewnić się, że nie jesteśmy w trakcie zmiany. 
    // unlock();

    // O ile nie chcesz "wychodzić" ze stadionu, nie zmniejszasz current_inside itp.
    // Po prostu się "pauzujesz" w tym miejscu.

    // Przywracamy domyślne działanie SIGTSTP i wznawiamy je:
    signal(SIGTSTP, SIG_DFL);
    raise(SIGTSTP);
}

/**
 * Handler SIGCONT – wznawiamy działanie.
 */
void sigcont_handler(int signo) {
    fprintf(stderr, "[FAN %d] SIGCONT => wznawiam!\n", getpid());

    g_preparing_suspend = 0;
    g_paused = 0;
    // Ewentualnie, jeśli konieczna jest jakaś re-inicjalizacja, można to zrobić tutaj.
}

/**
 * Główna logika
 */
int main(void) {
    // Podłącz się do zasobów
    msqid = msgget(BASE_IPC_KEY, 0666);
    if (msqid == -1) {
        perror("[FAN] msgget");
        exit(1);
    }
    shmid = shmget(BASE_IPC_KEY, sizeof(shared_data), 0666);
    if (shmid == -1) {
        perror("[FAN] shmget");
        exit(1);
    }
    semid = semget(BASE_IPC_KEY, 1, 0666);
    if (semid == -1) {
        perror("[FAN] semget");
        exit(1);
    }

    shm_ptr = (shared_data*) shmat(shmid, NULL, 0);
    if ((void*)shm_ptr == (void*)-1) {
        perror("[FAN] shmat");
        exit(1);
    }

    // Ustaw handlery sygnałów
    signal(SIGTSTP, sigtstp_handler);
    signal(SIGCONT, sigcont_handler);

    // Wylosuj drużynę i wiek
    srand(time(NULL) ^ getpid());
    int myTeam = rand() % 2;
    int myAge  = 18 + (rand() % 30);

    printf("[FAN %d] Próbuje wejść. team=%d, age=%d\n", getpid(), myTeam, myAge);

    // 1. Wyślij zapytanie (mtype=1)
    message_t req;
    req.mtype = 1;
    snprintf(req.mtext, sizeof(req.mtext), "%d %d", myTeam, myAge);
    if (msgsnd(msqid, &req, strlen(req.mtext)+1, 0) == -1) {
        perror("[FAN] msgsnd");
        exit(1);
    }

    // 2. Czekamy na odpowiedź (mtype=2)
    message_t rsp;
    if (msgrcv(msqid, &rsp, sizeof(rsp.mtext), 2, 0) == -1) {
        perror("[FAN] msgrcv");
        exit(1);
    }

    if (strcmp(rsp.mtext, "OK") == 0) {
        // Wpuszczony
        printf("[FAN %d] Otrzymałem 'OK'. Wchodzę!\n", getpid());
        g_inStadium = 1; // Jesteśmy w środku

        // Symulacja: czekamy, aż nastąpi ewakuacja (stadium_open=0)
        while (1) {
            // Jeśli pauza, dajmy pętli wytchnienie
            if (g_paused) {
                usleep(100000);  // czekamy, aż nadejdzie SIGCONT
                continue;
            }

            // Sprawdzaj, czy ewakuacja
            lock();
            int open = shm_ptr->stadium_open;
            unlock();

            if (!open) {
                // ewakuacja => wychodzimy
                break;
            }

            // krótka pauza
            sleep(1);
        }

        // Wyjście ze stadionu
        lock();
        shm_ptr->current_inside--;
        // Znajdź stację i zwolnij – w oryginalnym kodzie worker
        // przypisywał station_in_use i station_team. 
        // Dla uproszczenia: spróbujmy odszukać stację z naszą drużyną:
        for (int i = 0; i < NUM_STATIONS; i++) {
            if (shm_ptr->station_team[i] == myTeam && shm_ptr->station_in_use[i] > 0) {
                shm_ptr->station_in_use[i]--;
                if (shm_ptr->station_in_use[i] == 0) {
                    shm_ptr->station_team[i] = -1;
                }
                break;
            }
        }
        unlock();

        printf("[FAN %d] Ewakuuję się. Koniec.\n", getpid());
    } else {
        // Odrzucony
        printf("[FAN %d] Otrzymałem 'NO'. Rezygnuję.\n", getpid());
    }

    // Sprzątanie
    shmdt(shm_ptr);
    return 0;
}
