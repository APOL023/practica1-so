#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

volatile sig_atomic_t g_aviso = 0;    
volatile sig_atomic_t g_destruir = 0; 

void manejador_aviso(int s)    { g_aviso = 1; }
void manejador_destruir(int s) { g_destruir = 1; }

void espera(pid_t pid) {
    while (waitpid(pid, NULL, 0) == -1 && errno == EINTR);
}

void hoja(char nombre, int segundos, pid_t pid_ejec, pid_t pid_A) {
    signal(SIGUSR2, manejador_destruir);
    printf("Soy el proceso %c: mi pid es %d. Mi padre es %d. "
           "Mi abuelo es %d. Mi bisabuelo es %d\n",
           nombre, getpid(), getppid(), pid_A, pid_ejec);
    fflush(stdout);

    if (nombre == 'Z') {
        signal(SIGALRM, manejador_aviso);
        alarm(segundos);                
        while (!g_aviso) pause();        
        kill(pid_A, SIGUSR1);            
    }

    while (!g_destruir) pause();       
    printf("Soy %c (%d) y muero\n", nombre, getpid());
    exit(0);
}

void proceso_B(pid_t pid_ejec, pid_t pid_A, int segundos) {
    pid_t pid_B = getpid();
    pid_t hijos[3];
    char nombres[3] = {'X', 'Y', 'Z'};

    printf("Soy el proceso B: mi pid es %d. Mi padre es %d. Mi abuelo es %d\n",
           pid_B, getppid(), pid_ejec);
    fflush(stdout);
    signal(SIGUSR2, manejador_destruir);

    for (int i = 0; i < 3; i++) {
        hijos[i] = fork();
        if (hijos[i] == -1) { perror("fork"); exit(1); }
        if (hijos[i] == 0) hoja(nombres[i], segundos, pid_ejec, pid_A);
    }

    while (!g_destruir) pause();         

    for (int i = 2; i >= 0; i--) {
        kill(hijos[i], SIGUSR2);
        espera(hijos[i]);
    }
    printf("Soy B (%d) y muero\n", pid_B);
    exit(0);
}

void proceso_A(pid_t pid_ejec, int segundos) {
    pid_t pid_A = getpid();
    printf("Soy el proceso A: mi pid es %d. Mi padre es %d\n", pid_A, getppid());
    fflush(stdout);

    signal(SIGUSR1, manejador_aviso);   
    pid_t pid_B = fork();
    if (pid_B == -1) { perror("fork"); exit(1); }
    if (pid_B == 0) proceso_B(pid_ejec, pid_A, segundos);

    while (!g_aviso) pause();     

    char pidstr[16];
    snprintf(pidstr, sizeof(pidstr), "%d", (int)pid_ejec);
    pid_t pid_ps = fork();
    if (pid_ps == -1) { perror("fork"); exit(1); }
    if (pid_ps == 0) {
        execlp("pstree", "pstree", "-c", pidstr, NULL);
        perror("Error en exec");
        exit(1);
    }
    espera(pid_ps);

    kill(pid_B, SIGUSR2);           
    espera(pid_B);
    printf("Soy A (%d) y muero\n", pid_A);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <segundos>\n", argv[0]);
        exit(1);
    }
    int segundos = atoi(argv[1]);
    if (segundos <= 0) {
        fprintf(stderr, "Los segundos deben ser > 0\n");
        exit(1);
    }

    pid_t pid_ejec = getpid();
    printf("Soy el proceso ejec: mi pid es %d\n", pid_ejec);
    fflush(stdout);

    pid_t pid_A = fork();
    if (pid_A == -1) { perror("fork"); exit(1); }
    if (pid_A == 0) proceso_A(pid_ejec, segundos);

    espera(pid_A);
    printf("Soy ejec (%d) y muero\n", pid_ejec);
    return 0;
}