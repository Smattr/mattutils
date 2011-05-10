/* This code is designed to occupy all your RAM and (depending on your
 * operating system's page replacement algorithm) force all other processes'
 * pages to disk (swap). It is useful for troubleshooting OS VM code and
 * diagnosing dodgy RAM.
 */

#include <stdlib.h>
#include <stdio.h>

/* The number of bytes to use for the initial allocation. Each time an
 * allocation fails, the amount will be halved.
 */
#define INIT_ALLOC 1024*1024*1024 /* 1GB */

/* You'll need to SIGTERM this program to stop it. */

#ifdef __GNUC__
/* GCC-specific optimisation. */
int main(int argc, char** argv) __attribute__((noreturn));
#endif

typedef struct mem {
    unsigned int sz;
    /* Volatile prevents GCC NOPing the final loop. */
    volatile unsigned char *data;
    struct mem *next;
} mem_t;

int main(int argc, char** argv) {
    unsigned int sz;
    mem_t *head = 0, *last = 0, *tip = 0;

    printf("Grabbing all memory...");
    for (sz = INIT_ALLOC; sz > 1; sz /= 2) {
        while (1) {
            tip = (mem_t*) malloc(sizeof(mem_t));
            if (!tip) /* No memory for another structure. */
                break;

            tip->data = (unsigned char*) malloc(sizeof(unsigned char) * sz);
            if (!tip->data) { /* No memory for an allocation of this size. */
                free(tip);
                break;
            }

            /* Now we know the allocation succeeded. */
            tip->sz = sz;

            /* Update the linked-list pointers. */
            tip->next = 0;
            if (!last) {
                head = tip;
                last = tip;
            } else {
                last->next = tip;
                last = last->next;
            }
        }
    }
    printf("Done.\nNow just writing it all in a loop...\n");

    /* Just repeatedly touch all memory. With a sensible paging algorithm none
     * of the previously allocated memory will actually be paged in until this
     * point (i.e. you probably won't see your RAM usage increase until now).
     */
    while (1)
        for (last = head; last; last = last->next)
            for (sz = 0; sz < last->sz; ++sz)
                last->data[sz] = 0;

    /* We'll never reach this point. */
#ifndef __GNUC__
    return 0;
#endif
}
