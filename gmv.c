#include "gmv.h"

void init_pagetables(pagetable_t *pt) {
    for (int i = 0; i < NUM_PROCESSES; i++) {
        for (int j = 0; j < NUM_PAGES; j++) {
            pt[i].table[j].present = 0;
            pt[i].table[j].modified = 0;
            pt[i].table[j].referenced = 0;
            pt[i].table[j].frame_no = -1;
            pt[i].table[j].last_accessed = 0;
        }
    }
}

void init_frames(frame_t *frames) {
    for (int i = 0; i < NUM_FRAMES; i++) {
        frames[i].pid = -1;
        frames[i].page_no = -1;
    }
}

void update_access_time(pagetable_t *pt, int pid, int page, int time) {
    pt[pid].table[page].last_accessed = time;
}

void print_pagetable(pagetable_t *pt) {
    for (int i = 0; i < NUM_PAGES; i++) {
        page_entry_t *p = &pt->table[i];
        if (p->present) {
            printf("Página %2d → Quadro %2d | R=%d M=%d\n", i, p->frame_no, p->referenced, p->modified);
        }
    }
}

void run_2nCh(pagetable_t *pt, frame_t *frames, int *page_faults, int pid, int page, char access_type) {
    page_entry_t *entry = &pt->table[page];

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

    int best_class = 4, victim_index = -1;
    for (int i = 0; i < NUM_FRAMES; i++) {
        int vpid = frames[i].pid, vp = frames[i].page_no;
        page_entry_t *v = &pt_all[vpid].table[vp];
        int classe = (v->referenced << 1) | v->modified;
        if (classe < best_class) {
            best_class = classe;
            victim_index = i;
            if (classe == 0) break;
        }
    }

    if (victim_index != -1) {
        int vpid = frames[victim_index].pid, vpage = frames[victim_index].page_no;
        page_entry_t *victim = &pt_all[vpid].table[vpage];
        if (victim->modified)
            printf("Página suja: escrevendo P%d-pag%d no swap\n", vpid + 1, vpage);

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

    int candidata = -1;
    for (int i = 0; i < NUM_FRAMES; i++) {
        int vpid = frames[i].pid, vpage = frames[i].page_no;
        int found = 0;
        for (int j = 0; j < WS_K; j++) {
            if (ws_hist[vpid].acesso[j] == vpage) {
                found = 1; break;
            }
        }
        if (!found) {
            candidata = i; break;
        }
    }
    if (candidata == -1) candidata = 0;

    int vpid = frames[candidata].pid, vpage = frames[candidata].page_no;
    page_entry_t *victim = &pt[vpid].table[vpage];
    if (victim->modified)
        printf("Página suja: escrevendo P%d-pag%d no swap\n", vpid + 1, vpage);

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
    printf("→ Quadro perdido por P%d\n", vpid + 1);
}

void run_lru(pagetable_t *pt, frame_t *frames, int *page_faults, int pid, int page, char access_type, int time) {
    page_entry_t *entry = &pt[pid].table[page];

    if (entry->present) {
        entry->referenced = 1;
        if (access_type == WRITE) entry->modified = 1;
        update_access_time(pt, pid, page, time);
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
            update_access_time(pt, pid, page, time);
            return;
        }
    }

    int victim_index = -1, oldest = __INT_MAX__;
    for (int i = 0; i < NUM_FRAMES; i++) {
        int vpid = frames[i].pid, vpage = frames[i].page_no;
        page_entry_t *v = &pt[vpid].table[vpage];
        if (v->last_accessed < oldest) {
            oldest = v->last_accessed;
            victim_index = i;
        }
    }

    int vpid = frames[victim_index].pid, vpage = frames[victim_index].page_no;
    page_entry_t *victim = &pt[vpid].table[vpage];
    if (victim->modified)
        printf("Página suja: escrevendo P%d-pag%d no swap\n", vpid + 1, vpage);

    victim->present = 0;
    victim->referenced = 0;
    victim->modified = 0;
    victim->frame_no = -1;

    frames[victim_index].pid = pid;
    frames[victim_index].page_no = page;

    entry->present = 1;
    entry->referenced = 1;
    entry->modified = (access_type == WRITE);
    entry->frame_no = victim_index;
    update_access_time(pt, pid, page, time);

    printf("→ Page-fault gerado por P%d, página %d\n", pid + 1, page);
    printf("→ Quadro perdido por P%d\n", vpid + 1);
}

