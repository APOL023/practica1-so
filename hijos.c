#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

int shmid;
int *tabla = NULL;
int X, Y;

int creaComp(void) {
    shmid = shmget(IPC_PRIVATE, X * Y * sizeof(int), IPC_CREAT | 0666);
    if (shmid == -1) { perror("shmget"); return 1; }
    tabla = (int *) shmat(shmid, NULL, 0);
    if (tabla == (int *) -1) { perror("shmat"); return 1; }
    for (int k = 0; k < X * Y; k++) tabla[k] = 0;
    return 0;
}

void borraComp(void) {
    if (tabla != NULL) {
        if (shmdt((char *) tabla) < 0) perror("shmdt");
        if (shmctl(shmid, IPC_RMID, NULL) < 0) perror("shmctl");
        tabla = NULL;
    }
}

#define P(j, i) tabla[(j) * X + (i)]

void columna(int j, int nivel) {
    P(j, nivel) = getpid();            

    if (nivel < X - 1) {
        pid_t pid = fork();
        if (pid == -1) { perror("fork"); exit(1); }
        if (pid == 0) {
            columna(j, nivel + 1);
            exit(0);
        }
        wait(NULL);            
    } else {                         
        printf("Soy el subhijo %d, mis padres son:", getpid());
        for (int i = 0; i < X - 1; i++)
            printf("%s %d", i ? "," : "", P(j, i));
        printf("\n");
        fflush(stdout);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <x> <y>\n", argv[0]);
        exit(1);
    }
    X = atoi(argv[1]);
    Y = atoi(argv[2]);
    if (X < 1 || Y < 1) {
        fprintf(stderr, "x e y deben ser >= 1\n");
        exit(1);
    }
    if (creaComp()) exit(1);

    for (int j = 0; j < Y; j++) {
        pid_t pid = fork();
        if (pid == -1) { perror("fork"); borraComp(); exit(1); }
        if (pid == 0) {
            columna(j, 0);
            exit(0);
        }
    }
    while (wait(NULL) > 0);          

    printf("Soy el superpadre (%d): mis hijos finales son:", getpid());
    for (int j = 0; j < Y; j++)
        printf("%s %d", j ? "," : "", P(j, X - 1));
    printf("\n");

    borraComp();
    return 0;
}