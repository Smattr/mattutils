#include "alloc.h"
#include <string.h>
#include <stdint.h>

#define ALIGNMENT_CONSTRAINT 0

#define ROUND_DOWN(x, n) ((x) - (((uintptr_t)(x)) % (n)))
#define ROUND_UP(x, n)   ROUND_DOWN((x) + (n) - 1, n)

struct block {
    struct block *next;
    size_t size;
};

static struct block *free_list = NULL;

int add_block(void *ptr, size_t size) {
    if (size < sizeof(struct block)) {
        return -1;
    } else {
        struct block *b = (struct block*)ptr;
        b->next = free_list;
        b->size = size - sizeof(struct block);
        free_list = b;
    }
}

void *malloc(size_t size) {
    struct block *b, *prev;

    /* Shortcut edge case. */
    if (size == 0) {
        return NULL;
    }
#if 0
#if ALIGNMENT_CONSTRAINT == 0
    /* No alignment constraints on the returned pointer. The logic between this
     * and the case where an aligned pointer is required overlaps somewhat, but
     * the complexity introduced by the alignment case makes this code much
     * less readable.
     */

    for (prev = NULL, b = free_list; b != NULL; prev = b, b = b->next) {
        if (b->size >= size && b->size < size + sizeof(struct block) + 1) {
            /* Found a block that exactly satisfies this allocation (or so
             * closely that the remaining memory cannot form a block). */
            if (prev == NULL) {
                free_list = b->next;
            } else {
                prev->next = b->next;
            }
            return ((void*)b) + sizeof(struct block);
        } else if (b->size > size) {
            /* Found a block that satisfies this allocation with some left over. */
            struct block *remainder = (struct block*)(((void*)b) + sizeof(struct block) + size);
            remainder->size = b->size - sizeof(struct block) - size;
            b->size = size;
            remainder->next = b->next;
            if (prev == NULL) {
                free_list = remainder;
            } else {
                prev->next = remainder;
            }
            return ((void*)b) + sizeof(struct block);
        }
    }

else /* ALIGNMENT_CONSTRAINT != 0 */
    /* All returned pointers must be aligned to ALIGNMENT_CONSTRAINT. */
#endif
#endif

    for (prev = NULL, b = free_list; b != NULL; prev = b, b = b->next) {
        void *p;
        size_t aligned_size;

#if ALIGNMENT_CONSTRAINT
        /* This #if syntax is irritating and confusing. Putting the check for
         * ALIGNMENT_CONSTRAINT inside the following 'if' causes GCC to
         * erroneously generate a warning for division by zero when
         * ALIGNMENT_CONSTRAINT is 0, hence the need to avoid this with the
         * pre-processor.
         */
        if (size % ALIGNMENT_CONSTRAINT == 0) {
            /* This allocation needs to be aligned. Note we cheat a little bit
             * and assume that any non-aligned range does not need an aligned
             * start.
             */
            p = ROUND_UP(((void*)b) + sizeof(struct block), ALIGNMENT_CONSTRAINT);
        } else
#endif
        {
            /* No alignment required. */
            p = ((void*)b) + sizeof(struct block);
        }

        aligned_size = b->size - (p - (((void*)b) + sizeof(struct block)));

        if (aligned_size >= size && aligned_size < size + sizeof(struct block) + 1) {
            if (prev == NULL) {
                free_list = b->next;
            } else {
                prev->next = b->next;
            }
            return p;
        } else if (aligned_size > size) {
            struct block *remainder = (struct block*)(p + size);
            remainder->size = aligned_size - size - sizeof(struct block);
            b->size = size + b->size - aligned_size;
            remainder->next = b->next;
            if (prev == NULL) {
                free_list = remainder;
            } else {
                prev->next = remainder;
            }
            return p;
        }
    }

    return NULL;
}

void free(void *ptr) {
    if (ptr != NULL) {
        struct block *b = (struct block*)(ptr - sizeof(struct block));
        b->next = free_list;
        free_list = b;
    }
}

void *calloc(size_t num, size_t size) {
    /* TODO: Check/enforce alignment. */
    return malloc(num * size);
}

void *realloc(void *ptr, size_t size) {
    void *p;
    struct block *b;

    /* Optimise edge cases. */
    if (size == 0) {
        free(ptr);
        return;
    } else if (ptr == NULL) {
        return malloc(size);
    }

    /* TODO: optimise when size is less than the current size by shrinking the
     * existing allocation.
     */
    p = malloc(size);
    if (p == NULL) {
        return NULL;
    }

    b = (struct block*)(ptr - sizeof(struct block));
    (void)memcpy(p, ptr, b->size > size ? size : b->size);
    free(ptr);
    return p;
}
