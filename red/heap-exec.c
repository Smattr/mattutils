/* Determine whether we have an executable heap. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void ret(void) {}

int main(int argc, char **argv) {
    char *buffer = malloc(100);
    if (buffer == NULL) {
        perror("malloc");
        return EXIT_FAILURE;
    }
    memcpy(buffer, ret, 100);

    /* If the stack is not executable, the following will segfault. */
    ((void(*)(void))buffer)();

    printf("Your heap is executable\n");

    return 0;
}

