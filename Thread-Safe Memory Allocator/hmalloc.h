#ifndef HMALLOC_H
#define HMALLOC_H

#include <stddef.h>


// Husky Malloc Interface
// cs3650 Starter Code

typedef struct hm_stats {
    long pages_mapped;
    long pages_unmapped;
    long chunks_allocated;
    long chunks_freed;
    long free_length;
} hm_stats;

hm_stats *hgetstats();

void hprintstats();

void *hmalloc(size_t size);

void* hrealloc(void* prev, size_t bytes);

void hfree(void *item);

typedef struct free_node {
    size_t size;
    struct free_node *next;
} free_node;

free_node* free_list_init();

free_node* free_list_insert(free_node *node);

void *free_list_allocate(size_t size);

size_t free_list_remove_replace_current(free_node *prev, const free_node *cur, size_t size);

typedef struct data {
    size_t size;
} data;


#endif
