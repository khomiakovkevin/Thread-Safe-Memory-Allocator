#ifndef CS3650_BITMAP_T_H
#define CS3650_BITMAP_T_H

// Define the structure of a bitmap using a defaulting value
typedef struct bitmap_s {
    char bitmap[128];
} bitmap_t;

void init_bitmap(bitmap_t *bm);

int get_nth_bit(bitmap_t *bm, int n);

void set_nth_bit(bitmap_t *bm, int n);

void clear_nth_bit(bitmap_t *bm, int n);

int get_first_empty_bit(bitmap_t *bm, int max_size);

int get_first_nonempty_bit(bitmap_t *bm, int max_size);

#endif
