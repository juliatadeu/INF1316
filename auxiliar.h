#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define NUM_PROCESSES 4
#define NUM_FRAMES 16
#define NUM_PAGES 32

typedef enum{
    NRU,
    LRU,
    SECOND_CHANCE,
    WORKING_SET,
    INVALID_ALGORITHM
} algorithm_t;

algorithm_t get_algorithm(const char *algorithm_name);
void print_usage();
void print_usage_and_exit();
void print_page_fault(int gen_pid, int lost_pid, int page, int modified);