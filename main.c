#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include "gmv.h"

#define NUM_ROUNDS 6
#define MAX_LINE 100

const char *filenames[] = {
    "acessos/acessos_P1",
    "acessos/acessos_P2",
    "acessos/acessos_P3",
    "acessos/acessos_P4"
};int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <algoritmo>\n", argv[0]);
        fprintf(stderr, "Algoritmos suportados: nru, 2ch\n");
        return 1;
    }

    char *algoritmo = argv[1];

    pid_t children[NUM_PROCESSES];
    int pipes[NUM_PROCESSES][2];
    pagetable_t pt[NUM_PROCESSES];
    frame_t frames[NUM_FRAMES];
    int page_faults = 0;
    int faults_por_processo[NUM_PROCESSES] = {0};

    init_pagetables(pt);
    init_frames(frames);

    for (int i = 0; i < NUM_PROCESSES; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("Erro ao criar pipe");
            exit(1);
        }
    }

    for (int i = 0; i < NUM_PROCESSES; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("Erro no fork");
            exit(1);
        } else if (pid == 0) {
            close(pipes[i][0]);
            FILE *fp = fopen(filenames[i], "r");
            if (!fp) {
                perror("Erro ao abrir arquivo de acessos");
                exit(1);
            }
            raise(SIGSTOP);
            char linha[MAX_LINE];
            for (int j = 0; j < NUM_ROUNDS; j++) {
                if (fgets(linha, sizeof(linha), fp) != NULL) {
                    write(pipes[i][1], linha, strlen(linha));
                } else {
                    fprintf(stderr, "Arquivo %s terminou antes do esperado\n", filenames[i]);
                    break;
                }
                raise(SIGSTOP);
            }
            fclose(fp);
            close(pipes[i][1]);
            exit(0);
        } else {
            children[i] = pid;
            close(pipes[i][1]);
        }
    }

    usleep(100000);
    int current = 0;
    for (int r = 0; r < NUM_ROUNDS * NUM_PROCESSES; r++) {
        pid_t pid_atual = children[current];
        kill(pid_atual, SIGCONT);
        sleep(1);

        char buffer[MAX_LINE] = {0};
        int n = read(pipes[current][0], buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            printf("GMV recebeu de P%d: %s", current + 1, buffer);

            int page;
            char oper;
            if (sscanf(buffer, "%d %c", &page, &oper) == 2) {
                int before = page_faults;
                if (strcmp(algoritmo, "2ch") == 0) {
                    run_2nCh(&pt[current], frames, &page_faults, current, page, oper);
                } else if (strcmp(algoritmo, "nru") == 0) {
                    run_nru(pt, frames, &page_faults, current, page, oper);
                } else {
                    fprintf(stderr, "Algoritmo não reconhecido: %s\n", algoritmo);
                    exit(1);
                }
                if (page_faults > before)
                    faults_por_processo[current]++;
            }
        }

        kill(pid_atual, SIGSTOP);
        current = (current + 1) % NUM_PROCESSES;
    }

    for (int i = 0; i < NUM_PROCESSES; i++) {
        kill(children[i], SIGCONT);
    }

    for (int i = 0; i < NUM_PROCESSES; i++) {
        waitpid(children[i], NULL, 0);
        close(pipes[i][0]);
    }

    printf("\nTodos os processos filhos terminaram.\n");
    printf("Page faults totais: %d\n", page_faults);
    for (int i = 0; i < NUM_PROCESSES; i++) {
        printf("P%d teve %d page faults\n", i + 1, faults_por_processo[i]);
    }

    for (int i = 0; i < NUM_PROCESSES; i++) {
        printf("\nTabela de páginas do processo %d:\n", i + 1);
        print_pagetable(&pt[i]);
    }

    return 0;
}
