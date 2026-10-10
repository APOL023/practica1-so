#include <stdio.h>      
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUF 4096

void escribe(int fd, const char *b, ssize_t n) {
    ssize_t hecho = 0, r;
    while (hecho < n) {
        r = write(fd, b + hecho, n - hecho);
        if (r == -1) { perror("write"); exit(1); }
        hecho += r;
    }
}

void hijo(int fd_lectura, const char *archivo, int num) {
    char nombre[512], buf[BUF];
    ssize_t n;

    snprintf(nombre, sizeof(nombre), "%s.h%02d", archivo, num);
    int fd = open(nombre, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) { perror(nombre); exit(1); }

    while ((n = read(fd_lectura, buf, BUF)) > 0)
        escribe(fd, buf, n);

    close(fd);
    close(fd_lectura);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        write(2, "Uso: hacha <archivo> <tamano>\n", 30);
        exit(1);
    }
    long tam = atol(argv[2]);
    if (tam <= 0) {
        write(2, "El tamano debe ser > 0\n", 23);
        exit(1);
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) { perror(argv[1]); exit(1); }

    char buf[BUF];
    ssize_t n;
    int num = 0;

    while ((n = read(fd, buf, tam < BUF ? tam : BUF)) > 0) {
        int tubo[2];
        if (pipe(tubo) == -1) { perror("pipe"); exit(1); }

        pid_t pid = fork();
        if (pid == -1) { perror("fork"); exit(1); }
        if (pid == 0) {        
            close(tubo[1]);            
            close(fd);
            hijo(tubo[0], argv[1], num);
        }

        close(tubo[0]);            
        long restante = tam - n;
        escribe(tubo[1], buf, n);
        while (restante > 0 &&
               (n = read(fd, buf, restante < BUF ? restante : BUF)) > 0) {
            escribe(tubo[1], buf, n);
            restante -= n;
        }
        close(tubo[1]);            
        waitpid(pid, NULL, 0);
        num++;
    }

    close(fd);
    return 0;
}