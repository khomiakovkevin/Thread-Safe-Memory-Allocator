
#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>
#include <memory.h>

#include "hmalloc.h"

/*
  typedef struct hm_stats {
  long pages_mapped;
  long pages_unmapped;
  long chunks_allocated;
  long chunks_freed;
  long free_length;
  } hm_stats;
*/

const size_t PAGE_SIZE = 4096;
static hm_stats stats; // This initializes the stats to 0.
free_node *head = NULL;

/**
 * Uses mmap to map memory of given size.
 *
 * @param size
 * @return a pointer to that memory
 */
void
*map_memory(size_t size) {
    return mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
}

/**
 * Builds a new page as a free list and returns that free list.
 * @return the free list, NOT the offset
 */
free_node *
free_list_init() {
    free_node *new_list = map_memory(PAGE_SIZE);
    new_list->size = PAGE_SIZE;
    new_list->next = NULL;
    // Increase stats
    stats.pages_mapped += 1;
    return new_list;
}

/**
* Insert  the given node in sort order.
*
* @param node
* @return the node that precedes the inserted node in the list
*/
free_node *
free_list_insert(free_node *node) {
    free_node *prev = NULL;
    free_node *cur = head;
    while (cur < node && cur != NULL) {
        prev = cur;
        cur = cur->next;
    }
    // Current free_node element is the head of the list
    if (prev == NULL) {
        head = node;
        node->next = cur;
    } else { // Current free_node element is inside of the list
        prev->next = node;
        node->next = cur;
    }
    return prev;
}

/**
 * Given the previous list element and the current list element, attempts to construct a new list
 * element in the remaining space of give size, and inserts that element inside the linked list
 * where the current element was.
 *
 * @param prev
 * @param cur
 * @param size
 * @returns size given to the allocation
 */
size_t
free_list_remove_replace_current(free_node *prev, const free_node *cur, size_t size) {
    // Add a new free_node element to the remainder of the current
    free_node *next = cur->next;
    // We want to make sure that there are no gaps of missing data if we choose not to allocate
    size_t size_of_alloc = size;
    // If we have a remainder of free memory left, create the new struct there
    if (size + sizeof(free_node) < cur->size) {
        next = ((void *) cur) + size;
        next->size = cur->size - size;
        next->next = cur->next;
    } else {
        size_of_alloc = cur->size;
    }
    // Current free_node element is the head of the list
    if (prev == NULL) {
        head = next;
    } else { // Current free_node element is inside of the list
        prev->next = next;
    }
    return size_of_alloc;
}

/**
 * Allocates the given size of space and returns a void pointer to that memory after the header. If
 * necessary, will create a new page.
 *
 * @param size number of bytes to allocate, including the header
 * @return a void pointer to the allocated memory after the header
 */
void *
free_list_allocate(size_t size) {
    free_node *prev = NULL;
    free_node *cur = head;
    while (cur != NULL) {
        // We are going to return this
        if (cur->size >= size) {
            // Remove this free memory from the list
            size_t true_size = free_list_remove_replace_current(prev, cur, size);
            // Replace the free_node pointer with a data pointer
            data *d = (data *) cur;
            d->size = true_size;
            // Return data at an offset
            return ((void *) d) + sizeof(data);
        } else {
            // Visit the next element
            prev = cur;
            cur = cur->next;
        }
    }
    // We've reached the end of the list. Allocate a new page
    cur = free_list_init();
    prev = free_list_insert(cur);
    // Remove this memory from the list of free
    free_list_remove_replace_current(prev, cur, size);
    // Replace and return the data offset
    data *d = (data *) cur;
    d->size = size;
    // Return data at an offset
    return ((void *) d) + sizeof(data);
}

/**
 * Returns the number of elements in the free list
 * @return number of elements in the free list
 */
long
free_list_length() {
    long length = 0;
    free_node *cur = head;
    while (cur != NULL) {
        length += 1;
        cur = cur->next;
    }
    return length;
}

/**
 * Starting at the given element, traverses through the list and combines each element where their
 * memory addresses meet.
 *
 * @param start first element to inspect for combination
 */
void coalesce(free_node *start) {
    free_node *prev = start;
    free_node *cur = start->next;
    while (cur != NULL) {
        if (((void *) prev) + prev->size == cur) {
            // Combine the previous and current
            prev->size += cur->size;
            prev->next = cur->next;
            // Set the next
            cur = cur->next;
        } else {
            prev = cur;
            cur = cur->next;
        }
    }
}

/**
 * Creates a free_node struct at the given pointer and inserts it in the free list.
 *
 * @param d data pointer
 */
void
free_list_free_and_coalesce(data *d) {
    // Create a free_node struct and align it
    free_node *node = (free_node *) d;
    node->size = d->size;
    node->next = NULL;
    // Insert that node back into the list
    free_node *prev = free_list_insert(node);
    // If that node was the head, set correctly
    if (prev == NULL) {
        prev = head;
    }
    // Coalesce the data from that point on
    coalesce(prev);
}

hm_stats *
hgetstats() {
    stats.free_length = free_list_length();
    return &stats;
}

void
hprintstats() {
    stats.free_length = free_list_length();
    fprintf(stderr, "\n== husky malloc stats ==\n");
    fprintf(stderr, "Mapped:   %ld\n", stats.pages_mapped);
    fprintf(stderr, "Unmapped: %ld\n", stats.pages_unmapped);
    fprintf(stderr, "Allocs:   %ld\n", stats.chunks_allocated);
    fprintf(stderr, "Frees:    %ld\n", stats.chunks_freed);
    fprintf(stderr, "Freelen:  %ld\n", stats.free_length);
}

static
size_t
div_up(size_t xx, size_t yy) {
    // This is useful to calculate # of pages
    // for large allocations.
    size_t zz = xx / yy;

    if (zz * yy == xx) {
        return zz;
    } else {
        return zz + 1;
    }
}

void *
hmalloc(size_t size) {
    // Handle stats
    size += sizeof(data);
    // Enforce 16 byte minimum
    if (size < sizeof(free_node)) {
        size = sizeof(free_node);
    }
    stats.chunks_allocated += 1;
    if (size < PAGE_SIZE) {
        // Initialize if the head does not exist
        if (head == NULL) {
            head = free_list_init();
        }
        return free_list_allocate(size);
    } else {
        // Allocate at intervals of PAGE_SIZE
        size_t num_pages = div_up(size, PAGE_SIZE);
        data *d = map_memory(num_pages * PAGE_SIZE);
        d->size = num_pages * PAGE_SIZE;
        // Keep track of stats
        stats.pages_mapped += num_pages;
        // Return data at an offset
        return ((void *) d) + sizeof(data);
    }
}

/**
 * Returns the size of allocated memory.
 *
 * @param item the address of that memory
 * @return the size of the allocation
 */
size_t
get_size(void* item) {
    data* d = item - sizeof(data);
    return d->size;
}

void*
hrealloc(void* prev, size_t bytes) {
    void *alloc = hmalloc(bytes);
    if (get_size(prev) < bytes) {
        memcpy(alloc, prev, bytes);
        hfree(prev);
        return alloc;
    } else {
        hfree(alloc);
        return prev;
    }
}

void
hfree(void *item) {
    stats.chunks_freed += 1;
    // Extract the data size from the pointer
    data *d = item - sizeof(data);
    if (d->size < PAGE_SIZE) {
        free_list_free_and_coalesce(d);
    } else {
        size_t num_pages = div_up(d->size, PAGE_SIZE);
        stats.pages_unmapped += num_pages;
        munmap(item, d->size);
    }
}
