#include "gmv.h"

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

void run_nru(pagetable_t *pt_all, frame_t *frames, int *page_faults, int pid, int page, char access_type) {
    page_entry_t *entry = &pt_all[pid].table[page];

    if (entry->present) {
        entry->referenced = 1;
        if (access_type == WRITE) entry->modified = 1;
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

    // Substituição NRU:
    int best_class = 4;
    int victim_index = -1;

    for (int i = 0; i < NUM_FRAMES; i++) {
        int vp = frames[i].page_no;
        int vpid = frames[i].pid;
        page_entry_t *v = &pt_all[vpid].table[vp];

        int classe = (v->referenced << 1) | v->modified;
        if (classe < best_class) {
            best_class = classe;
            victim_index = i;
            if (classe == 0) break;
        }
    }

    if (victim_index != -1) {
        int victim_pid = frames[victim_index].pid;
        int victim_page = frames[victim_index].page_no;
        page_entry_t *victim = &pt_all[victim_pid].table[victim_page];

        if (victim->modified)
            printf("Página suja: escrevendo P%d-pag%d no swap\n", victim_pid + 1, victim_page);

        victim->present = 0;
        victim->modified = 0;
        victim->referenced = 0;
        victim->frame_no = -1;

        frames[victim_index].pid = pid;
        frames[victim_index].page_no = page;

        entry->present = 1;
        entry->referenced = 1;
        entry->modified = (access_type == WRITE);
        entry->frame_no = victim_index;
    }
}

void run_ws(pagetable_t *pt, frame_t *frames, int *page_faults, int pid, int page, char access_type, working_set_t *ws_hist) {
    page_entry_t *entry = &pt[pid].table[page];

    // Atualiza histórico
    ws_hist[pid].acesso[ws_hist[pid].ponteiro] = page;
    ws_hist[pid].ponteiro = (ws_hist[pid].ponteiro + 1) % WS_K;

    if (entry->present) {
        entry->referenced = 1;
        if (access_type == WRITE) entry->modified = 1;
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

    // Substituição com base em páginas fora da janela
    int candidata = -1;
    for (int i = 0; i < NUM_FRAMES; i++) {
        int vpid = frames[i].pid;
        int vpage = frames[i].page_no;
        int na_janela = 0;
        for (int j = 0; j < WS_K; j++) {
            if (ws_hist[vpid].acesso[j] == vpage) {
                na_janela = 1;
                break;
            }
        }
        if (!na_janela) {
            candidata = i;
            break;
        }
    }

    if (candidata == -1) candidata = 0; // fallback

    int victim_pid = frames[candidata].pid;
    int victim_page = frames[candidata].page_no;
    page_entry_t *victim = &pt[victim_pid].table[victim_page];

    if (victim->modified)
        printf("Página suja: escrevendo P%d-pag%d no swap\n", victim_pid + 1, victim_page);

    victim->present = 0;
    victim->referenced = 0;
    victim->modified = 0;
    victim->frame_no = -1;

    frames[candidata].pid = pid;
    frames[candidata].page_no = page;

    entry->present = 1;
    entry->referenced = 1;
    entry->modified = (access_type == WRITE);
    entry->frame_no = candidata;

    printf("→ Page-fault gerado por P%d, página %d\n", pid + 1, page);
    printf("→ Quadro perdido por P%d\n", victim_pid + 1);
}


void print_pagetable(pagetable_t *pt) {
    for (int i = 0; i < NUM_PAGES; i++) {
        page_entry_t *p = &pt->table[i];
        if (p->present) {
            printf("Página %2d → Quadro %2d | R=%d M=%d\n", i, p->frame_no, p->referenced, p->modified);
        }
    }
}