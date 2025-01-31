#ifndef STADIUM_H
#define STADIUM_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <pthread.h>  // bo w "worker.c" używamy wątków

// Klucz bazowy do IPC
#define BASE_IPC_KEY  0x1234

// Parametry
#define NUM_STATIONS       3      // liczba stanowisk
#define STATION_CAPACITY   3      // ile osób równocześnie na jednym stanowisku
#define MAX_STADIUM_CAP    1000   // globalny limit kibiców na stadionie

#define QUEUE_SIZE         50     // wielkość kolejki do każdej stacji

// Struktura opisująca kibica (np. w kolejce do stacji)
typedef struct {
    int team;  // drużyna (0 lub 1)
    int age;
    int pid;   // PID (lub inny identyfikator) kibica
} fan_info;

// Struktura pamięci współdzielonej
typedef struct {
    int stadium_open;      // 1 = otwarty, 0 = ewakuacja
    int allow_entrance;    // 1 = wpuszczamy, 0 = STOP
    int current_inside;    // ilu kibiców w środku (na całym stadionie)

    // Stanowiska:
    int station_in_use[NUM_STATIONS];  // ile osób w danej chwili
    int station_team[NUM_STATIONS];    // drużyna, którą obsługuje stacja (-1 = żadna)

    // Kolejki do każdej stacji (FIFO)
    fan_info station_queue[NUM_STATIONS][QUEUE_SIZE];
    int queue_count[NUM_STATIONS];
    int queue_front[NUM_STATIONS];
    int queue_rear[NUM_STATIONS];
} shared_data;

// Struktura komunikatu (System V message queue)
typedef struct {
    long mtype;
    char mtext[128];
} message_t;

// union semun do operacji semctl
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

// Prototypy funkcji z utils.c
void semWait(int semid, int semnum);
void semSignal(int semid, int semnum);

#endif
