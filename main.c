#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

#define NUM_PROCESSES 4
#define NUM_ROUNDS 4
#define MAX_LINE 100

const char *filenames[] = {
    "acessos/acessos_P1",
    "acessos/acessos_P2",
    "acessos/acessos_P3",
    "acessos/acessos_P4"
};

int main() {
    pid_t children[NUM_PROCESSES];
    int pipes[NUM_PROCESSES][2]; // pipe[i][0]: read end (pai), pipe[i][1]: write end (filho)

    // Cria os pipes
    for (int i = 0; i < NUM_PROCESSES; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("Erro ao criar pipe");
            exit(1);
        }
    }

    // Cria os processos filhos
    for (int i = 0; i < NUM_PROCESSES; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("Erro no fork");
            exit(1);
        } else if (pid == 0) {
            // Processo filho
            // Fecha extremidade de leitura
            close(pipes[i][0]);

            FILE *fp = fopen(filenames[i], "r");
            if (!fp) {
                perror("Erro ao abrir arquivo de acessos");
                exit(1);
            }

            raise(SIGSTOP); // pausa inicial

            char linha[MAX_LINE];
            for (int j = 0; j < NUM_ROUNDS; j++) {
                if (fgets(linha, sizeof(linha), fp) != NULL) {
                    write(pipes[i][1], linha, strlen(linha));
                } else {
                    fprintf(stderr, "Arquivo %s terminou antes do esperado\n", filenames[i]);
                    break;
                }
                raise(SIGSTOP); // pausa até próxima rodada
            }

            fclose(fp);
            close(pipes[i][1]);
            exit(0);
        } else {
            children[i] = pid;
            close(pipes[i][1]); // pai fecha extremidade de escrita
        }
    }

    usleep(100000); // espera filhos pausarem

    // Rodadas do escalonador Round Robin
    int current = 0;
    for (int r = 0; r < NUM_ROUNDS * NUM_PROCESSES; r++) {
        pid_t pid_atual = children[current];
        kill(pid_atual, SIGCONT); // libera o filho

        sleep(1); // tempo para execução

        // Pai lê o que o filho escreveu no pipe
        char buffer[MAX_LINE] = {0};
        int n = read(pipes[current][0], buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            printf("GMV recebeu de P%d: %s", current + 1, buffer);
        }

        kill(pid_atual, SIGSTOP); // pausa novamente
        current = (current + 1) % NUM_PROCESSES;
    }

    // Libera os filhos para terminarem
    for (int i = 0; i < NUM_PROCESSES; i++) {
        kill(children[i], SIGCONT);
    }

    // Espera os filhos finalizarem
    for (int i = 0; i < NUM_PROCESSES; i++) {
        waitpid(children[i], NULL, 0);
        close(pipes[i][0]); // fecha leitura do pipe
    }

    printf("Todos os processos filhos terminaram.\n");
    return 0;
}
