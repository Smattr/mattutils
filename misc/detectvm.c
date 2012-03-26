#include <stdio.h>
#include <stdint.h>

/* This code determines whether it is running in a virtual machine or not. It
 * employs the same technique as Joanna Rutkowska's Red Pill
 * (http://invisiblethings.org/papers/redpill.html).
 */

int main(int argc, char **argv) {
    uintptr_t idtr[2];

    /* Read the interrupt descriptor table register (IDTR) into idtr. */
    asm ("sidt %[ptr]" : [ptr]"=m"(idtr));

    /* The IDT is at an address greater than 0xd0000000 in all the popular
     * hypervisors.
     */
    if (idtr[0] > 0xd0000000) {
        printf("You are being virtualised.\n");
        return 1;
    } else {
        printf("You are running natively.\n");
        return 0;
    }
}
