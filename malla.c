#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define ESPERA 60   


void columna(int nivel, int x) {
    if (nivel < x) {
        pid_t pid = fork();
        if (pid == -1) { perror("fork"); exit(1); }
        if (pid == 0) {           
            columna(nivel + 1, x);
            exit(0);
        }
        wait(NULL);                
    } else {
        sleep(ESPERA);           
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <filas x> <columnas y>\n", argv[0]);
        exit(1);
    }
    int x = atoi(argv[1]);
    int y = atoi(argv[2]);
    if (x < 1 || y < 1) {
        fprintf(stderr, "x e y deben ser >= 1\n");
        exit(1);
    }

    for (int j = 0; j < y; j++) {
        pid_t pid = fork();
        if (pid == -1) { perror("fork"); exit(1); }
        if (pid == 0) {          
            columna(1, x);
            exit(0);
        }
    }
    while (wait(NULL) > 0);       
    return 0;
}