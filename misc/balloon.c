/* Memory ballooning application.
 *
 * This program is designed to work like a virtual machine memory balloon
 * driver. It pins pages in memory (prevents the operating system swapping them
 * out to disk). The reason for doing this is typically putting pressure on
 * another running application or restricting the amount of available physical
 * memory in a system. Note that, unlike a balloon driver, the memory is not
 * made available to anyone else. It is just horded internally until you want
 * to release it.
 *
 * To use, run with no arguments and then use the following keys. Note that you
 * will have to run it as root (or under sudo) to pin any significant amount of
 * memory.
 *  right arrow - occupy STEP_SMALL more memory
 *  left arrow - occupy STEP_SMALL less memory
 *  up arrow - occupy STEP_MEDIUM more memory
 *  down arrow - occupy STEP_MEDIUM less memory
 *  page up - occupy STEP_LARGE more memory
 *  page down - occupy STEP_LARGE less memory
 *  q - quit (and release all memory)
 *
 * Legal: This code is in the public domain. Do whatever you like with it.
 *
 * If you have any questions or want some new functionality, please email me.
 *  Matthew Fernandez <matthew.fernandez@gmail.com>
 */

#include <assert.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <termios.h>
#include <unistd.h>

/* Increments in which to increase and decrease pinned memory. Note that these
 * are actually implemented as loops that repeatedly operate on 4KB chunks.
 * This is pretty inefficient, but it means we don't need to care about
 * fragmentation.
 */
static const long long STEP_SMALL = 1024LL * 1024;        /* 1MB */
static const long long STEP_MEDIUM = 50LL * 1024 * 1024;  /* 50MB */
static const long long STEP_LARGE = 500LL * 1024 * 1024;  /* 500MB */

/* Allocate and pin a page in memory. Returns a pointer to the page or NULL on
 * failure.
 */
static void *lock_page(void) {
    void *p = mmap(NULL, getpagesize(), PROT_READ|PROT_WRITE,
        MAP_PRIVATE|MAP_ANONYMOUS|MAP_LOCKED, 0, 0);
    if (p == MAP_FAILED)
        return NULL;

    *((void**)p) = NULL;
    return p;
}

/* Unpin and deallocate a locked page. */
static int unlock_page(void *page) {
    return munmap(page, getpagesize());
}

/* A collection of pages pinned in memory. We track the pages as a linked-list,
 * storing the next pointers inside the pages themselves in the first word.
 */
typedef struct {
    void *head;           /* Start of the list. */
    void *last;           /* Last member of the list. */
    long long occupancy;  /* Number of bytes the list covers. */
} page_collection_t;

/* Output the current amount of memory pinned in this collection. */
static void pc_print_occupancy(page_collection_t *pc) {
    if (pc->occupancy >= 1024LL * 1024 * 1024)
        mvprintw(0, 0, "%0.2fGB pinned\n", (double)pc->occupancy / 1024 / 1024 / 1024);
    else if (pc->occupancy >= 1024LL * 1024)
        mvprintw(0, 0, "%0.2fMB pinned\n", (double)pc->occupancy / 1024 / 1024);
    else if (pc->occupancy >= 1024)
        mvprintw(0, 0, "%0.2fKB pinned\n", (double)pc->occupancy / 1024);
    else
        mvprintw(0, 0, "%lldB pinned\n", pc->occupancy);

    refresh();
}

/* Increase the amount of pinned memory by 'bytes' if possible. Returns the
 * actual number of bytes it was increased by.
 */
static long long pc_gain(page_collection_t *pc, long long bytes) {
    assert(bytes > 0);
    long long i;
    for (i = 0; i < bytes; i += getpagesize()) {
        void *p = lock_page();
        if (p == NULL) {
            /* We failed to pin another page. */
            break;
        }
        if (pc->head == NULL) {
            /* There is nothing in the list currently. */
            pc->head = p;
            pc->last = p;
        } else {
            /* Common case append. */
            *((void**)pc->last) = p;
            pc->last = p;
        }
    }
    pc->occupancy += i;
    return i;
}

/* Decrease the amount of pinned memory by 'bytes' if possible. Returns the
 * actual number of bytes it was decreased by.
 */
static long long pc_lose(page_collection_t *pc, long long bytes) {
    assert(bytes > 0);
    long long i;
    for (i = 0; i < bytes; i += getpagesize()) {
        if (pc->head == NULL) {
            /* We have no pages to drop. */
            break;
        } else {
            void *next = *((void**)pc->head);
            if (unlock_page(pc->head) != 0) {
                /* We failed to unpin a page. Who knows why. */
                break;
            }
            pc->head = next;
            if (pc->head == NULL) {
                /* We just unpinned the last page. */
                pc->last = NULL;
            }
        }
    }
    pc->occupancy -= i;
    return i;
}

int main(void) {
    page_collection_t pages = { NULL };

    /* Initialise ncurses. */
    initscr();
    cbreak();
    keypad(stdscr, TRUE);
    noecho();

    int c;
    pc_print_occupancy(&pages);
    while ((c = getch()) != 'q') {
        switch (c) {

            case KEY_LEFT:
                pc_lose(&pages, STEP_SMALL);
                break;

            case KEY_RIGHT:
                pc_gain(&pages, STEP_SMALL);
                break;

            case KEY_DOWN:
                pc_lose(&pages, STEP_MEDIUM);
                break;

            case KEY_UP:
                pc_gain(&pages, STEP_MEDIUM);
                break;

            case KEY_NPAGE:
                pc_lose(&pages, STEP_LARGE);
                break;

            case KEY_PPAGE:
                pc_gain(&pages, STEP_LARGE);
                break;

        }
        pc_print_occupancy(&pages);
    }

    /* De-initialise ncurses. */
    endwin();

    return 0;
}
