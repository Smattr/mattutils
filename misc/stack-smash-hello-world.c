#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* This code demonstrates a stack smashing attack. Note that the layout of the
 * stack is very compiler (and architecture) dependent. You may have to adjust
 * the OFFSET value, change compiler flags or perform tweaks to avoid GCC's
 * stack smashing detection.
 */
#define OFFSET 57

int smash(void) {
    char bp;

    strcpy(OFFSET + &bp, "world\n");

    return;
}

int main(int argc, char** argv) {
    char my_string[100];
    strcpy(my_string, "Hello ");

    printf("%s", my_string);
    smash();
    printf("%s", my_string);

    return 0;
}
