#include "stadium.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uzycie: %s <PID_WORKER>\n", argv[0]);
        return 1;
    }
    pid_t worker_pid = (pid_t)atoi(argv[1]);
    printf("[MANAGER] PID worker-a: %d\n", worker_pid);

    while (1) {
        printf("\n[MANAGER] Menu:\n");
        printf("1 - STOP (SIGUSR1)\n");
        printf("2 - RESUME (SIGUSR2)\n");
        printf("3 - EWAKUACJA (SIGINT)\n");
        printf("4 - Wyjdz\n");
        printf("Wybór: ");

        int choice;
        if (scanf("%d", &choice) != 1) {
            printf("Błąd wczytywania.\n");
            break;
        }

        switch (choice) {
            case 1:
                kill(worker_pid, SIGUSR1);
                printf("[MANAGER] STOP wyslano\n");
                break;
            case 2:
                kill(worker_pid, SIGUSR2);
                printf("[MANAGER] RESUME wyslano\n");
                break;
            case 3:
                kill(worker_pid, SIGINT);
                printf("[MANAGER] EWAKUACJA wyslano\n");
                break;
            case 4:
                printf("[MANAGER] Koniec.\n");
                return 0;
            default:
                printf("Nieznana opcja.\n");
                break;
        }
    }

    return 0;
}
