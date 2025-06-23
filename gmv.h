#ifndef GMV_H
#define GMV_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

// Definições de constantes
#define NUM_PROCESSES 4   // Número de processos
#define NUM_FRAMES 16     // Número total de quadros de página
#define NUM_PAGES 32      // Número total de páginas
#define MAX_ACCESS 100    // Número máximo de acessos por processo

// Definindo os tipos de acessos
#define READ  'R'
#define WRITE 'W'

// Estrutura para representar uma página
typedef struct {
    int present;    // Se a página está na memória (1 = sim, 0 = não)
    int modified;   // Se a página foi modificada (1 = sim, 0 = não)
    int referenced; // Se a página foi referenciada (1 = sim, 0 = não)
    int frame_no;   // Número do quadro onde a página está armazenada
} page_entry_t;

// Estrutura para representar a tabela de páginas de um processo
typedef struct {
    page_entry_t table[NUM_PAGES];  // Tabela de páginas com as entradas
} pagetable_t;

// Estrutura para representar um quadro de memória
typedef struct {
    int pid;      // ID do processo que possui a página
    int page_no;  // Número da página armazenada no quadro
} frame_t;

// Funções para os algoritmos de substituição de páginas

// Algoritmo NRU (Not Recently Used)
void run_nru(pagetable_t *pt, frame_t *frames, int *page_faults);

// Algoritmo Segunda Chance
void run_2nCh(pagetable_t *pt, frame_t *frames, int *page_faults, int pid, int page, char access_type);

// Algoritmo LRU (Least Recently Used) / Aging
void run_lru(pagetable_t *pt, frame_t *frames, int *page_faults);

// Algoritmo Working Set
void run_ws(pagetable_t *pt, frame_t *frames, int *page_faults, int k);

// Função de inicialização das tabelas de páginas
void init_pagetables(pagetable_t *pt);

// Função de inicialização dos quadros de memória
void init_frames(frame_t *frames);

// Função para obter o próximo algoritmo a ser utilizado (NRU, Segunda Chance, etc.)
int get_algorithm(char *algorithm_name);

// Função para realizar a leitura do arquivo de acessos
void read_access_file(const char *filename, int *accesses, char *operations);

// Função para imprimir a tabela de páginas de um processo
void print_pagetable(pagetable_t *pt);

#endif // GMV_H
