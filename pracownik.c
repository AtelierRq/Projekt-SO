#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#define QUEUE_KEY 1234567

typedef struct {
    long type;       // Typ wiadomości
    int command;     // Komenda od kierownika (1 = wstrzymanie, 2 = wpuszczanie, 3 = opuszczanie)
    int team_id;     // ID drużyny kibica (opcjonalne)
} Message;

// Liczba stanowisk i miejsc na stanowiskach
#define STANOWISKA 3
#define MIEJSCA 3

// Struktura dla kolejki kibiców
typedef struct {
    int team_id[1000]; // Kolejka kibiców
    int front;         // Indeks początku kolejki
    int rear;          // Indeks końca kolejki
    int count;         // Liczba kibiców w kolejce
} Queue;

// Funkcje do obsługi kolejki
void enqueue(Queue *queue, int team_id) {
    if (queue->count < 1000) {
        queue->team_id[queue->rear] = team_id;
        queue->rear = (queue->rear + 1) % 1000;
        queue->count++;
    }
}

int dequeue(Queue *queue) {
    if (queue->count > 0) {
        int team_id = queue->team_id[queue->front];
        queue->front = (queue->front + 1) % 1000;
        queue->count--;
        return team_id;
    }
    return -1; // Pusta kolejka
}

// Obsługa wpuszczania kibiców
void process_security_check(int msgid, Queue *queue) {
    while (queue->count >= MIEJSCA) {
        int current_team = queue->team_id[queue->front]; // Drużyna kibica

        // Wpuść 3 kibiców na stanowisko
        printf("Pracownik: Wpuszczam 3 kibiców z drużyny %d.\n", current_team);
        for (int i = 0; i < MIEJSCA; i++) {
            if (queue->count > 0) {
                int team_id = dequeue(queue);

                // Wysłanie komunikatu do kibica
                Message to_kibic;
                to_kibic.type = 2; // Typ wiadomości dla kibica
                to_kibic.command = 2; // Komenda wpuszczania
                to_kibic.team_id = team_id;

                if (msgsnd(msgid, &to_kibic, sizeof(to_kibic) - sizeof(long), 0) == -1) {
                    perror("Blad wysylania wiadomosci do kibica");
                }
            }
        }
    }
}

// Obsługa sygnałów od kierownika
void handle_signal(int msgid, Message *msg, Queue *queue, int *pause) {
    if (msg->command == 1) {
        printf("Pracownik: Wstrzymuje wpuszczanie kibicow.\n");
        *pause = 1;
    } else if (msg->command == 2) {
        printf("Pracownik: Rozpoczyna wpuszczanie kibicow.\n");
        *pause = 0;
        process_security_check(msgid, queue);
    } else if (msg->command == 3) {
        printf("Pracownik: Rozpoczyna opuszczanie stadionu.\n");
        *pause = 1;

        // Informowanie kibiców o opuszczeniu stadionu
        while (queue->count > 0) {
            int team_id = dequeue(queue);

            Message to_kibic;
            to_kibic.type = 2; // Typ wiadomości dla kibica
            to_kibic.command = 3; // Komenda opuszczania
            to_kibic.team_id = team_id;

            if (msgsnd(msgid, &to_kibic, sizeof(to_kibic) - sizeof(long), 0) == -1) {
                perror("Blad wysylania sygnalu do kibica");
            }
        }
    }
}

int main() {
    int msgid = msgget(QUEUE_KEY, 0666);
    if (msgid == -1) {
        perror("Blad otwierania kolejki komunikatow");
        exit(1);
    }

    printf("Pracownik: Oczekuje na sygnaly...\n");

    Queue queue = {.front = 0, .rear = 0, .count = 0};

    Message msg;
    int pause = 1; // Domyślnie wstrzymanie

    while (1) {
        // Oczekiwanie na wiadomość
        if (msgrcv(msgid, &msg, sizeof(msg) - sizeof(long), -1, 0) == -1) {
            perror("Blad odbioru wiadomosci");
            exit(1);
        }

        if (msg.type == 1) {
            // Wiadomość od kierownika
            handle_signal(msgid, &msg, &queue, &pause);
        } else if (msg.type == 2 && !pause) {
            // Kibice zgłaszają się do kolejki, tylko jeśli nie ma pauzy
            enqueue(&queue, msg.team_id);
        }

        if (msg.command == 3 && queue.count == 0) {
            printf("Pracownik: Zakonczono dzialanie.\n");
            break;
        }
    }

    return 0;
}
