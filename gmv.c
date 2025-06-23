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


void print_pagetable(pagetable_t *pt) {
    for (int i = 0; i < NUM_PAGES; i++) {
        page_entry_t *p = &pt->table[i];
        if (p->present) {
            printf("Página %2d → Quadro %2d | R=%d M=%d\n", i, p->frame_no, p->referenced, p->modified);
        }
    }
}

void run_lru(pagetable_t *pt, frame_t *frames, int *page_faults) {
    int i, j;
    int page_to_replace = -1;
    int frame_to_replace = -1;
    int oldest_access_time = -1;

    // Percorre os quadros de memória para encontrar o quadro a ser substituído
    for (i = 0; i < NUM_FRAMES; i++) {
        if (frames[i].page_no != -1) {  // Se o quadro está ocupado
            int pid = frames[i].pid;
            int page_no = frames[i].page_no;

            // Verifica o tempo do último acesso da página nesse quadro
            int access_time = pt[pid].table[page_no].last_accessed;

            // Se essa página foi menos recentemente usada, ela é a candidata a substituição
            if (oldest_access_time == -1 || access_time < oldest_access_time) {
                oldest_access_time = access_time;
                page_to_replace = page_no;
                frame_to_replace = i;
            }
        }
    }

    // Se encontrou uma página a ser substituída, faz a substituição
    if (page_to_replace != -1) {
        // Substitui a página no quadro
        printf("Substituindo a página %d do processo %d pelo quadro %d\n", page_to_replace, frames[frame_to_replace].pid, frame_to_replace);

        // Atualiza a página no quadro
        frames[frame_to_replace].page_no = page_to_replace;
        frames[frame_to_replace].pid = pt[frames[frame_to_replace].pid].table[page_to_replace].frame_no;

        // Marca um page-fault
        (*page_faults)++;
    }
    else {
        printf("Nenhuma página encontrada para substituição (todas estão em uso recente).\n");
    }
}

void update_access_time(pagetable_t *pt, int pid, int page_no, int current_time) {
    pt[pid].table[page_no].last_accessed = current_time;
}