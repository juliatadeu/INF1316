#ifndef GMV_H
#define GMV_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#define NUM_PROCESSES 4
#define NUM_FRAMES 16
#define NUM_PAGES 32
#define MAX_ACCESS 100
#define READ  'R'
#define WRITE 'W'
#define WS_K 4 // janela de working set

typedef struct {
    int present;
    int modified;
    int referenced;
    int frame_no;
    int last_accessed;
} page_entry_t;

typedef struct {
    page_entry_t table[NUM_PAGES];
} pagetable_t;

typedef struct {
    int pid;
    int page_no;
} frame_t;

typedef struct {
    int acesso[WS_K];
    int ponteiro;
} working_set_t;

void init_pagetables(pagetable_t *pt);
void init_frames(frame_t *frames);

void run_nru(pagetable_t *pt_all, frame_t *frames, int *page_faults, int pid, int page, char access_type);
void run_2nCh(pagetable_t *pt, frame_t *frames, int *page_faults, int pid, int page, char access_type);
void run_ws(pagetable_t *pt, frame_t *frames, int *page_faults, int pid, int page, char access_type, working_set_t *ws_hist);
void run_lru(pagetable_t *pt, frame_t *frames, int *page_faults, int pid, int page, char access_type, int time);

void print_pagetable(pagetable_t *pt);
void update_access_time(pagetable_t *pt, int pid, int page, int time);

#endif
