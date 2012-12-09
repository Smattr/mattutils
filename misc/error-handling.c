/* Handling errors in C is a royal pain in the arse. To be more specific,
 * because you don't have exceptions or proper control structures for error
 * handling, robust code is polluted with verbose inline error handling. You
 * see recurring patterns like:
 *
 *  x = might_fail(...);
 *  if (x != 0) {
 *      // Handle error
 *      ...
 *  }
 *
 * Recently there's been a lot of discussion about error handling in general
 * and how try-catch isn't the right mechanism for this either. The sad fact is
 * that it also introduces a lot of noise around your core logic. The nicest
 * way I've seen error handling done is with CPS in Scala. I thought I'd try my
 * hand at out-of-line error handling in C and here's what I came up with.
 * Obviously it has some drawbacks, but if you've read this far I assume you
 * are capable of discerning them for yourself.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER 100

int copy(char *in, char *out) {

#define fopen(args...) \
    /* Error handler for fopen. */ \
    ({ \
        FILE *_fd = fopen(args); \
        if (_fd == NULL) { \
            if (fout) fclose(fout); \
            return -1; \
        } \
        _fd;\
    })
#define malloc(args...) \
    /* Error handler for malloc. */ \
    ({ \
        void *_ptr = malloc(args); \
        if (_ptr == NULL) { \
            if (fin) fclose(fin); \
            if (fout) fclose(fout); \
            if (sout) free(sout); \
            return -1; \
        } \
        _ptr; \
    })

    /* We need to initialise our variables for the error handlers to function
     * correctly. A small price to pay.
     */
    FILE *fin = NULL, *fout = NULL;
    char *sin = NULL, *sout = NULL;

    /* Now we can consolidate some nice, uncluttered program logic. */
    fin = fopen(in, "r");
    fout = fopen(out, "w");
    sin = (char*)malloc(sizeof(char) * BUFFER);
    sout = (char*)malloc(sizeof(char) * BUFFER);
    size_t rd;

    while (rd = fread(sin, 1, BUFFER, fin)) {
        memcpy(sout, sin, rd);
        fwrite(sout, 1, rd, fout);
    }
    free(sout);
    free(sin);
    fclose(fout);
    fclose(fin);

    return 0;

#undef fopen
#undef malloc
}

int main(int argc, char **argv) {
    if (copy(argv[1], argv[2])) {
        printf("Copy failed\n");
    }
    return 0;
}
