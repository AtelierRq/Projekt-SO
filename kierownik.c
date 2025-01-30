#include "stadium.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uzycie: %s <PID_WORKER>\n", argv[0]);
        exit(1);
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
            printf("Blad wczytywania.\n");
            break;
        }
        switch (choice) {
            case 1:
                kill(worker_pid, SIGUSR1);
                printf("[MANAGER] Wyslano SIGUSR1 => STOP.\n");
                break;
            case 2:
                kill(worker_pid, SIGUSR2);
                printf("[MANAGER] Wyslano SIGUSR2 => RESUME.\n");
                break;
            case 3:
                kill(worker_pid, SIGINT);
                printf("[MANAGER] Wyslano SIGINT => EWAKUACJA.\n");
                break;
            case 4:
                printf("[MANAGER] Koncze.\n");
                return 0;
            default:
                printf("Nieznana opcja.\n");
        }
    }
    return 0;
}
