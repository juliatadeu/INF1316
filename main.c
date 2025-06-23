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
};

void init_pagetables(pagetable_t *pt) {
    for (int i = 0; i < NUM_PROCESSES; i++) {
        for (int j = 0; j < NUM_PAGES; j++) {
            pt[i].table[j].present = 0;    // Página não está na memória inicialmente
            pt[i].table[j].modified = 0;   // Página não foi modificada
            pt[i].table[j].referenced = 0; // Página não foi referenciada
            pt[i].table[j].frame_no = -1;  // Página não tem quadro de memória
        }
    }
}

void init_frames(frame_t *frames) {
    for (int i = 0; i < NUM_FRAMES; i++) {
        frames[i].pid = -1;   // Nenhum processo usando o quadro inicialmente
        frames[i].page_no = -1; // Nenhuma página alocada ao quadro inicialmente
    }
}

void run_nru(pagetable_t *pt, frame_t *frames, int *page_faults) {
    int i, j;
    int page_to_replace = -1;
    int frame_to_replace = -1;

    // Percorre a tabela de páginas de todos os processos
    for (i = 0; i < NUM_PROCESSES; i++) {
        // Percorre todas as páginas do processo i
        for (j = 0; j < NUM_PAGES; j++) {
            // Verifica se a página não foi referenciada e não foi modificada
            if (pt[i].table[j].referenced == 0 && pt[i].table[j].modified == 0) {
                // Encontra a primeira página que atende aos critérios
                page_to_replace = j;
                break;
            }
        }
        
        if (page_to_replace != -1) {
            // Encontrou uma página para substituir
            for (int k = 0; k < NUM_FRAMES; k++) {
                if (frames[k].pid == i && frames[k].page_no == page_to_replace) {
                    frame_to_replace = k;
                    break;
                }
            }

            if (frame_to_replace != -1) {
                // Substitui a página
                printf("Substituindo a página %d do processo %d pelo quadro %d\n", page_to_replace, i, frame_to_replace);
                pt[i].table[page_to_replace].referenced = 0;  // Resetar o bit de referência
                pt[i].table[page_to_replace].modified = 0;   // Resetar o bit de modificação

                // Caso tenha sido uma página modificada, registra no quadro
                if (frames[frame_to_replace].page_no != -1) {
                    // Se o quadro já contiver uma página, esta será substituída
                    printf("A página %d foi substituída no quadro %d\n", frames[frame_to_replace].page_no, frame_to_replace);
                }
                frames[frame_to_replace].page_no = page_to_replace;
                frames[frame_to_replace].pid = i;

                // Registra um page fault
                (*page_faults)++;
            }

            break; // Sai do loop assim que encontrar a página para substituir
        }
    }

    // Se não encontrou nenhuma página para substituir, o algoritmo não fez nenhuma substituição
    if (page_to_replace == -1) {
        printf("Nenhuma página encontrada para substituição (todas estão referenciadas ou modificadas).\n");
    }
}


int main() {
    pid_t children[NUM_PROCESSES];
    int pipes[NUM_PROCESSES][2]; // pipe[i][0]: read end (pai), pipe[i][1]: write end (filho)
    pagetable_t pt[NUM_PROCESSES];
    frame_t frames[NUM_FRAMES];
    int page_faults = 0;

    init_pagetables(pt);
    init_frames(frames);

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

            int page;
            char oper;
            if (sscanf(buffer, "%d %c", &page, &oper) == 2) {
                run_2nCh(&pt[current], frames, &page_faults, current, page, oper);
            }
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

    printf("\nTodos os processos filhos terminaram.\n");
    printf("Page faults totais: %d\n", page_faults);
    for (int i = 0; i < NUM_PROCESSES; i++) {
        printf("\nTabela de páginas do processo %d:\n", i + 1);
        print_pagetable(&pt[i]);
    }

    return 0;
}
