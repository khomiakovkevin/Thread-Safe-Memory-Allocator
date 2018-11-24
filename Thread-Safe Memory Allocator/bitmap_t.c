
#include <assert.h>
#include <stdio.h>
#include <strings.h>
#include <stdbool.h>
#include "bitmap_t.h"

void
init_bitmap(bitmap_t *bm) {
    for (int ii = 0; ii < 128; ++ii) {
        bm->bitmap[ii] = 0;
    }
}

int
get_nth_bit(bitmap_t *bm, int n) {
    // Make sure that n can fit
    assert(n < 128 * 8);
    // Get n/8th character
    char c = bm->bitmap[n / 8];
    // Returns the n % 8th bit (from LSB to MSB) in the character
    return (c >> n % 8) & 1U;
}

void
mod_nth_bit(bitmap_t *bm, int n, bool set) {
    // Make sure that n can fit
    assert(n < 128 * 8);
    if (set) {
        bm->bitmap[n / 8] |= 1UL << n % 8;
    } else {
        bm->bitmap[n / 8] &= ~(1UL << n % 8);
    }
}

void
set_nth_bit(bitmap_t *bm, int n) {
    mod_nth_bit(bm, n, true);
}

void
clear_nth_bit(bitmap_t *bm, int n) {
    mod_nth_bit(bm, n, false);
}

int
check_bits(bitmap_t *bm, int max_size, bool empty) {
    // Get the number of bytes that we need to iterate over
    int max_iter_size = (max_size + 7) / 8;
    // The number of characters we convert at a time
    int increment_value = sizeof(long);
    for (int ii = 0; ii < max_iter_size; ii += increment_value) {
        long l;
        if (empty) {
            l = ~*((long *) &(bm->bitmap[ii]));
        } else {
            l = *((long *) &(bm->bitmap[ii]));
        }
        int for_fucks_sake = ffsl(l);
        if (for_fucks_sake != 0) {
            for_fucks_sake += ii * sizeof(long);
            if (for_fucks_sake <= max_size) {
                return for_fucks_sake - 1;
            }
            return -1;
        }
    }
    return -1;
}

int
get_first_empty_bit(bitmap_t *bm, int max_size) {
    return check_bits(bm, max_size, true);
}

int
get_first_nonempty_bit(bitmap_t *bm, int max_size) {
    return check_bits(bm, max_size, false);
}

#define MAIN 0 // Turn off debugging by setting this to 0
#if MAIN

void
print_first_x_chars(bitmap_t *bm, int x) {
    for (int ii = 0; ii < x; ++ii) {
        char c = bm->bitmap[ii];
        for (int i = 0; i < 8; ++i) {
            printf("%d", ((c << i) & 0x80) != 0);
        }
        printf("\n");
    }
}

int
main(int argc, char *argv[]) {
    bitmap_t map;
    init_bitmap(&map);
    // Set the 9-th bit to 1
    set_nth_bit(&map, 9);
    assert(get_nth_bit(&map, 9) == 1);
    // Check that setting the bit again keeps it as 1
    set_nth_bit(&map, 9);
    assert(get_nth_bit(&map, 9) == 1);
    assert(get_first_empty_bit(&map, 128 * 8) == 0);
    assert(get_first_nonempty_bit(&map, 128 * 8) == 9);
    // Check that setting another bit in the same character makes no bad mods
    set_nth_bit(&map, 10);
    assert(get_nth_bit(&map, 9) == 1);
    assert(get_first_empty_bit(&map, 128 * 8) == 0);
    assert(get_first_nonempty_bit(&map, 128 * 8) == 9);
    assert(get_nth_bit(&map, 10) == 1);
    // Testing the first character
    set_nth_bit(&map, 0);
    assert(get_nth_bit(&map, 0) == 1);
    assert(get_first_empty_bit(&map, 128 * 8) == 1);
    assert(get_first_nonempty_bit(&map, 128 * 8) == 0);
    // Testing the last character
    set_nth_bit(&map, 128 * 8 - 1);
    assert(get_nth_bit(&map, 128 * 8 - 1) == 1);
    assert(get_first_empty_bit(&map, 128 * 8) == 1);
    // Asset that clearing works
    clear_nth_bit(&map, 9);
    assert(get_nth_bit(&map, 9) == 0);
    // Assert that clearing an already cleared bit works
    clear_nth_bit(&map, 9);
    assert(get_nth_bit(&map, 9) == 0);
    // Assert that accessing nonempty bit on next long size works
    init_bitmap(&map);
    set_nth_bit(&map, 65);
    assert(get_nth_bit(&map, 65) == 1);
    assert(get_first_nonempty_bit(&map, 100) == 65);
    // Testing max size
    assert(get_first_nonempty_bit(&map, 64) == -1);
    assert(get_first_nonempty_bit(&map, 66) == 65);
    clear_nth_bit(&map, 65);
    assert(get_first_nonempty_bit(&map, 128 * 8 - 1) == -1);
    return 0;
}

#endif
