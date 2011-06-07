/* This code demonstrates how to use an LD_PRELOAD hack to override built-in
 * libraries. It's mainly a reminder to myself of how this stuff works for when
 * I need to write one in a hurry.
 *
 * Note, you can avoid the dlopen call if you're including stdio anyway by
 * calling dlsym(RTLD_NEXT, ...), but I've left this in here for a more general
 * example.
 *
 * To compile:
 *  gcc -shared -ldl -fPIC -o puts-ldpreload-example.so puts-ldpreload-example.c
 * To run:
 *  LD_PRELOAD=./puts-ldpreload-example.so some_program_using_puts
 */

#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>

#if !defined(__pic__) && !defined(__PIC__)
    #warning "It looks like you have not used the compiler flags to generate position-independent code (-fPIC for GCC). Your resulting library may not function correctly."
#endif

int puts(const char* str) {
    void* stdio = 0;
    int (*real_puts)(const char*);
    int result;

    /* Dynamically load the real libc. */
    stdio = dlopen("/lib/lib.so.6", RTLD_LAZY);

    if (stdio) {

        /* Locate the real puts function. */
        real_puts = dlsym(stdio, "puts");

        if (dlerror()) {
            printf("Failed to load symbol puts from libc.\n");
            result = -1;
        } else {
            /* Instrument whatever intercession you like here. */
            real_puts("Intercepted: ");
            result = real_puts(str);
        }

        /* Clean up. */
        dlclose(stdio);

    } else {
        fprintf(stderr, "Failed to open underlying libc.\n");
        result = -1;
    }

    return result;
}
