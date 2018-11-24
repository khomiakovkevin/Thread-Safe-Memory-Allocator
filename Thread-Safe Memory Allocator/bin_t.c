#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include "bin_t.h"

void
check_rv(long rv) {
    if (rv == -1) {
        perror("oops");
        fflush(stdout);
        fflush(stderr);
        abort();
    }
}

void
*map_memory(size_t size) {
    void *ret = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    check_rv((long) ret);
    return ret;
}

/**
 * Maps a new page of memory for a bin which uses chunks of the given size. Returns that bin's
 * address pointer.
 *
 * @param size the size of each chunk in the bin
 * @param tid thread id
 * @return the pointer to a new block of memory
 */
bin_t
*init_small_bin(size_t size, pthread_t tid) {
    bin_t *bin = map_memory(PAGE_SIZE);
    init_bitmap(&bin->bitmap);
    bin->tid = tid;
    bin->is_large = false;
    bin->bin_size = size;
    bin->next = NULL;
    pthread_mutex_init(&bin->mutex, 0);
    return bin;
}

/**
 * Maps a custom amount of memory for a large bin. Returns that bin's address pointer.
 *
 * @param size the size of the full bin
 * @param tid thread id
 * @return the pointer to a new block of memory
 */
bin_t
*init_large_bin(size_t size, pthread_t tid) {
    bin_t *bin = map_memory(size);
    bin->tid = tid;
    bin->is_large = true;
    bin->size_large = size;
    pthread_mutex_init(&bin->mutex, 0);
    return bin;
}

/**
 * Gets the maximum number of data chunks a bin can hold.
 *
 * @param bin used to determine the size of each chunk
 * @return the amount of chunks that can fit inside this memory
 */
int
get_max_item_count(bin_t *bin) {
    size_t space = PAGE_SIZE - sizeof(bin_t);
    return (int) (space / bin->bin_size);
}

/**
 * Given an index into a bit, returns the n-th available chunk of memory from that bin. This memory
 * might already have data but that does not matter. Offset by the size of the header.
 *
 * @param bin to pull memory from
 * @param index 0-indexed chunk of memory
 * @return pointer to that memory
 */
void
*get_memory_at_nth_index(bin_t *bin, int index) {
    void *alloc = (void *) bin;
    alloc += sizeof(bin_t) + index * bin->bin_size;
    return alloc;
}

/**
 * Gets the available memory after the head, or returns available memory of a new bin.
 *
 * @param head the bin list head
 * @param max_size max number of items in the bin
 * @return the void pointer to free memory
 */
void
*get_memory_after_head(bin_t *head, int max_size) {
    bin_t *cur = head->next;
    int count = 0;
    while (cur != NULL && count < 10) {
        if (pthread_mutex_trylock(&cur->mutex) == 0) {
            int available_memory_index = get_first_empty_bit(&cur->bitmap, max_size);
            if (available_memory_index != -1) {
                set_nth_bit(&cur->bitmap, available_memory_index);
                pthread_mutex_unlock(&cur->mutex);
                return get_memory_at_nth_index(cur, available_memory_index);
            }
            pthread_mutex_unlock(&cur->mutex);
        }
        cur = cur->next;
        count += 1;
    }
    // If we've reached here, this means that we haven't found free memory in the first 10 bins
    pthread_mutex_lock(&head->mutex);
    bin_t *next = init_small_bin(head->bin_size, head->tid);
    next->next = head->next;
    head->next = next;
    pthread_mutex_unlock(&head->mutex);
    // Grab the 0-th index memory
    pthread_mutex_lock(&next->mutex);
    set_nth_bit(&next->bitmap, 0);
    void *memory = get_memory_at_nth_index(next, 0);
    pthread_mutex_unlock(&next->mutex);
    return memory;
}

/**
 * Designed to be run at the head of the list of bins, returns the first available memory address it
 * finds. If there's no existing memory, it creates a new bin, inserts the bin just following the
 * head, and returns available memory from that bin.
 *
 * @param bin head bin
 * @return void pointer to available memory
 */
void
*get_memory(bin_t *bin) {
    // Check if there's any available memory in the "head". This is where we start checking and
    // iterating through each of our equally-sized bins. If no memory is available
    int max_size = get_max_item_count(bin);
    pthread_mutex_lock(&bin->mutex);
    int available_memory_index = get_first_empty_bit(&bin->bitmap, max_size);
    if (available_memory_index != -1) {
        set_nth_bit(&bin->bitmap, available_memory_index);
        void *memory = get_memory_at_nth_index(bin, available_memory_index);
        pthread_mutex_unlock(&bin->mutex);
        return memory;
    } else {
        pthread_mutex_unlock(&bin->mutex);
    }

    // If there's valid memory down the chain, return it. Otherwise, build a new bin and grab the
    // available memory from that bin.
    return get_memory_after_head(bin, max_size);
}

void
free_small_bin(bin_t *bin, int index_of_offset, bin_t *head) {
    pthread_mutex_lock(&bin->mutex);
    clear_nth_bit(&bin->bitmap, index_of_offset);
    // Be sure to remove the bin if it's completely empty and not the head
    if (bin != head) {
        int max_size = get_max_item_count(bin);
        // Check if the bin is empty
        if (get_first_nonempty_bit(&bin->bitmap, max_size + 1) == -1) {
            bin_t *prev = head;
            bin_t *cur = head->next;
            while (bin != cur && cur != NULL) {
                prev = cur;
                cur = cur->next;
            }
            // If the previous gets freed
            if (pthread_mutex_trylock(&prev->mutex)) {
                pthread_mutex_unlock(&bin->mutex);
                return;
            }
            if (cur != NULL) {
                prev->next = cur->next;
            } else {
                prev->next = NULL;
            }
            pthread_mutex_unlock(&prev->mutex);
            munmap(bin, PAGE_SIZE);
            return;
        }
    }
    pthread_mutex_unlock(&bin->mutex);
}

void
free_large_bin(bin_t *bin) {
    munmap(bin, bin->size_large);
}