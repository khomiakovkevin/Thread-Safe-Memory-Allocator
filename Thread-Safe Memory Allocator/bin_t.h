#ifndef CS3650_BIN_T_H
#define CS3650_BIN_T_H

#include <stddef.h>
#include <pthread.h>
#include <stdbool.h>
#include "bitmap_t.h"

#define PAGE_SIZE 4096

void *map_memory(size_t size);

typedef struct bin_s {
    // For small bins only
    bitmap_t bitmap;
    size_t bin_size;
    // Shared
    pthread_t tid;
    pthread_mutex_t mutex;
    struct bin_s *next;
    // For large bins only
    bool is_large;
    size_t size_large;
} bin_t;

bin_t *init_small_bin(size_t size, pthread_t tid);

bin_t *init_large_bin(size_t size, pthread_t tid);

void free_small_bin(bin_t *bin, int index_of_offset, bin_t *head);

void free_large_bin(bin_t *bin);

void *get_memory(bin_t *bin);

#endif //CS3650_BIN_T_H
