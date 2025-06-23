#include "gmv.h"

void init_pagetables(pagetable_t *pt) {
    for (int p = 0; p < NUM_PROCESSES; p++) {
        for (int i = 0; i < NUM_PAGES; i++) {
            pt[p].table[i].present = 0;
            pt[p].table[i].modified = 0;
            pt[p].table[i].referenced = 0;
            pt[p].table[i].frame_no = -1;
        }
    }
}

void init_frames(frame_t *frames) {
    for (int i = 0; i < NUM_FRAMES; i++) {
        frames[i].pid = -1;
        frames[i].page_no = -1;
    }
}

void run_2nCh(pagetable_t *pt, frame_t *frames, int *page_faults, int pid, int page, char access_type) {
    page_entry_t *entry = &pt->table[page];

    if (entry->present) {
        entry->referenced = 1;
        if (access_type == WRITE)
            entry->modified = 1;
        return;
    }

    printf("PAGE FAULT: Processo %d, página %d\n", pid + 1, page);
    (*page_faults)++;

    for (int i = 0; i < NUM_FRAMES; i++) {
        if (frames[i].pid == -1) {
            frames[i].pid = pid;
            frames[i].page_no = page;

            entry->present = 1;
            entry->referenced = 1;
            entry->modified = (access_type == WRITE);
            entry->frame_no = i;
            return;
        }
    }

    static int ponteiro = 0;
    while (1) {
        int i = ponteiro;
        int victim_pid = frames[i].pid;
        int victim_page = frames[i].page_no;
        page_entry_t *victim = &pt[victim_pid].table[victim_page];

        if (victim->referenced == 1) {
            victim->referenced = 0;
            ponteiro = (ponteiro + 1) % NUM_FRAMES;
        } else {
            if (victim->modified)
                printf("Página suja: escrevendo P%d-pag%d no swap\n", victim_pid + 1, victim_page);

            victim->present = 0;
            victim->modified = 0;
            victim->referenced = 0;
            victim->frame_no = -1;

            frames[i].pid = pid;
            frames[i].page_no = page;

            entry->present = 1;
            entry->referenced = 1;
            entry->modified = (access_type == WRITE);
            entry->frame_no = i;

            ponteiro = (ponteiro + 1) % NUM_FRAMES;
            break;
        }
    }
}

void print_pagetable(pagetable_t *pt) {
    for (int i = 0; i < NUM_PAGES; i++) {
        page_entry_t *p = &pt->table[i];
        if (p->present) {
            printf("Página %2d → Quadro %2d | R=%d M=%d\n", i, p->frame_no, p->referenced, p->modified);
        }
    }
}