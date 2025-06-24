#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include "gmv.h"
#include "auxiliar.h"

#define MAX_LINE 100

const char *filenames[] = {
    "acessos/acessos_P1",
    "acessos/acessos_P2",
    "acessos/acessos_P3",
    "acessos/acessos_P4"
};

void print_usage_and_exit() {
    printf("Uso correto: ./main <algoritmo> <número de rodadas>\n");
    printf("Algoritmos válidos: nru, 2ch, ws, lru\n");
    exit(1);
}

algorithm_t get_algorithm(const char *algorithm_name) {
    if (strcmp(algorithm_name, "nru") == 0) return NRU;
    if (strcmp(algorithm_name, "lru") == 0) return LRU;
    if (strcmp(algorithm_name, "2ch") == 0) return SECOND_CHANCE;
    if (strcmp(algorithm_name, "ws") == 0) return WORKING_SET;
    return INVALID_ALGORITHM;
}
void print_usage() {
    printf("Uso: ./main <algoritmo> <número de rodadas>\n");
    printf("Algoritmos disponíveis: nru, lru, 2ch, ws\n");
    printf("Exemplo: ./main nru 10\n");
}

// Função para registrar o page fault com informações detalhadas
void print_page_fault(int gen_pid, int lost_pid, int page, int modified) {
    printf("Page fault gerado por P%d: Página %d\n", gen_pid + 1, page);
    printf("P%d perdeu o quadro de página\n", lost_pid + 1);
    if (modified) {
        printf("Page fault ocasionou uma escrita de página modificada (suja)\n");
    }
    printf("\n");
}

int main(int argc, char *argv[]) {
    // Verifica se o número de argumentos está correto
    if (argc != 3) {
        print_usage_and_exit();
    }

    char *algoritmo = argv[1];
    int NUM_ROUNDS = atoi(argv[2]);

    // Verifica se o número de rodadas é válido
    if (NUM_ROUNDS <= 0) {
        printf("Erro: o número de rodadas deve ser um número inteiro positivo.\n");
        print_usage_and_exit();
    }

    // Verifica se o algoritmo é válido
    int algoritmo_valido = get_algorithm(algoritmo);
    if (algoritmo_valido == 0) {
        printf("Erro: Algoritmo inválido '%s'.\n", algoritmo);
        print_usage_and_exit();
    }

    // Exibe informações sobre o algoritmo e o número de rodadas
    printf("\nAlgoritmo de Substituição: %s\n", algoritmo);
    printf("Rodadas executadas: %d\n\n", NUM_ROUNDS);

    // Inicializa os componentes necessários
    pid_t children[NUM_PROCESSES];
    int pipes[NUM_PROCESSES][2];
    pagetable_t pt[NUM_PROCESSES];
    frame_t frames[NUM_FRAMES];
    working_set_t ws_hist[NUM_PROCESSES] = {0};
    int page_faults = 0;
    int faults_por_processo[NUM_PROCESSES] = {0};
    int tempo_global = 0;

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
            raise(SIGSTOP); // Pausa o processo até o escalonador continuar
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

    usleep(100000); // Pequena pausa para garantir que os processos estejam prontos

    int current = 0;
    for (int r = 0; r < NUM_ROUNDS * NUM_PROCESSES; r++) {
        pid_t pid_atual = children[current];
        kill(pid_atual, SIGCONT); // Continua o processo
        sleep(1);

        char buffer[MAX_LINE] = {0};
        int n = read(pipes[current][0], buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            printf("GMV recebeu de P%d: %s", current + 1, buffer);

            int page;
            char oper;
            int page_modified = 0; // Para indicar se foi uma página modificada

            if (sscanf(buffer, "%d %c", &page, &oper) == 2) {
                int before = page_faults;

                // Executa o algoritmo conforme a escolha do usuário
                if (algoritmo_valido == 2) {  // Segundo Chance
                    run_2nCh(&pt[current], frames, &page_faults, current, page, oper);
                } else if (algoritmo_valido == 1) {  // NRU
                    run_nru(pt, frames, &page_faults, current, page, oper);
                } else if (algoritmo_valido == 3) {  // Working Set
                    run_ws(pt, frames, &page_faults, current, page, oper, ws_hist);
                } else if (algoritmo_valido == 4) {  // LRU
                    run_lru(pt, frames, &page_faults, current, page, oper, tempo_global);
                }

                // Se houve um page fault, verifica se foi uma página modificada
                if (page_faults > before) {
                    faults_por_processo[current]++;
                    // Verifica se foi uma página "suja"
                    if (oper == 'W') {
                        page_modified = 1;
                    }

                    // Registra a informação do page fault
                    print_page_fault(current, current, page, page_modified);
                }
            }
        }
        tempo_global++;

        kill(pid_atual, SIGSTOP); // Pausa o processo
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