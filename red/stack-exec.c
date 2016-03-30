/* Determine whether we have an executable stack.
 *
 * Notes:
 *  - To get GCC to mark stack executable `-z execstack`
 */

#include <stdio.h>
#include <string.h>

void ret(void) {}

int main(int argc, char **argv) {
    char buffer[100];
    memcpy(buffer, ret, sizeof buffer);

    /* If the stack is not executable, the following will segfault. */
    ((void(*)(void))buffer)();

    printf("Your stack is executable\n");

    return 0;
}
