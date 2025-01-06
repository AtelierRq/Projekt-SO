#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void wejscie_na_stadion() {
    printf("Kibic: Wchodze na stadion.\n");
}

void opuszczenie_stadionu() {
    printf("Kibic: Opuszczam stadion.\n");
}

int main() {
    while (1) {
        wejscie_na_stadion();
        sleep(2); // symulacja pobytu na stadionie
        opuszczenie_stadionu();
        break;
    }

    return 0;
}
