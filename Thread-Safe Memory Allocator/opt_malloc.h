#ifndef CS3650_OPT_MALLOC_H
#define CS3650_OPT_MALLOC_H

#include <stddef.h>
#include "bin_t.h"

#define NUM_OF_BIN_SIZES 19
static size_t BIN_SIZES[NUM_OF_BIN_SIZES] = {4, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 384,
                                             512, 768, 1024, 1536, 2048, 3072};

typedef struct bins_list {
    bin_t *bins[NUM_OF_BIN_SIZES];
} bins_list;

typedef struct arena_list {
    pthread_t tid;
    bins_list *bins;
    struct arena_list *next;
} arena_list;

void *opt_malloc(size_t bytes);

void opt_free(void *item);

void *opt_realloc(void *prev, size_t bytes);

#endif //CS3650_OPT_MALLOC_H
