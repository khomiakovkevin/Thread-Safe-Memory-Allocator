#include <stdlib.h>
#include <sys/mman.h>
#include <memory.h>
#include <stdio.h>
#include <assert.h>
#include "bin_t.h"
#include "opt_malloc.h"

// Thread-local linked list of bins
__thread bins_list *bin_list;
static arena_list *arenas;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * ================================================================
 * Bin lists
 * ================================================================
 */

void
init_arena() {
    arena_list *a = arenas;
    if (a == NULL) {
        arenas = map_memory(sizeof(arena_list));
        arenas->tid = pthread_self();
        arenas->bins = bin_list;
        arenas->next = NULL;
    } else {
        while (a->next != NULL) {
            a = a->next;
        }
        arena_list *next_arena = map_memory(sizeof(arena_list));
        next_arena->tid = pthread_self();
        next_arena->bins = bin_list;
        next_arena->next = NULL;
        a->next = next_arena;
    }
}

void
init_bins() {
    bin_list = map_memory(sizeof(bins_list));
    pthread_mutex_lock(&mutex);
    init_arena();
    pthread_mutex_unlock(&mutex);

    for (int bi = 0; bi < NUM_OF_BIN_SIZES; ++bi) {
        bin_t *bin = init_small_bin(BIN_SIZES[bi], pthread_self());
        bin_list->bins[bi] = bin;
    }
}

bin_t
*get_bin(void *item) {
    long mem = (long) item;
    mem = mem >> 12; // Shift right 12 bits (e.g. 0xBFD4C231 --> 0xBFD4C)
    mem = mem << 12; // Shift left 12 bits (e.g. 0xBFD4C --> 0xBFD4C000)
    return (bin_t *) mem;
}

/**
 * ================================================================
 * Memory allocation
 * ================================================================
 */

bin_t
*get_bin_from_size(size_t bytes, bins_list *list) {
    for (int ii = 0; ii < NUM_OF_BIN_SIZES; ++ii) {
        // Since the bytes list is sorted from lowest to highest, this will short circuit on the
        // first available bin
        if (bytes <= list->bins[ii]->bin_size) {
            return list->bins[ii];
        }
    }
    return NULL;
}

void
*opt_malloc(size_t bytes) {
    if (bin_list == NULL) {
        init_bins();
    }
    bin_t *bin = get_bin_from_size(bytes, bin_list);
    if (bin != NULL) {
        return get_memory(bin);
    } else {
        bin = init_large_bin(bytes, pthread_self());
        return (void *) bin + sizeof(bin_t);
    }
}

bins_list
*get_bins_list(bin_t *bin) {
    pthread_t tid = pthread_self();
    if (tid == bin->tid) {
        return bin_list;
    } else {
        arena_list *a = arenas;
        tid = a->tid;
        while (tid != bin->tid) {
            a = a->next;
            tid = a->tid;
        }
        return a->bins;
    }
}

void
opt_free(void *item) {
    bin_t *bin = get_bin(item);
    if (bin->is_large) {
        free_large_bin(bin);
    } else {
        size_t offset = item - (void *) bin - sizeof(bin_t);
        int index_of_alloc = (int) (offset / bin->bin_size);
        pthread_mutex_lock(&mutex);
        bins_list *thread_bins = get_bins_list(bin);
        bin_t *head = get_bin_from_size(bin->bin_size, thread_bins);
        pthread_mutex_unlock(&mutex);
        free_small_bin(bin, index_of_alloc, head);
    }
}

void
*opt_realloc(void *prev, size_t bytes) {
    if (bytes == 0) {
        opt_free(prev);
        return prev;
    }
    void *alloc = opt_malloc(bytes);
    bin_t *b = get_bin(prev);
    // Allocation size of given previous
    size_t prev_size = b->is_large ? b->size_large : b->bin_size;
    if (prev_size < bytes) {
        memcpy(alloc, prev, prev_size);
        opt_free(prev);
        return alloc;
    } else {
        opt_free(alloc);
        return prev;
    }
}