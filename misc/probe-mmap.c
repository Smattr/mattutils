/* Linux lays out a process's address space in several regions, including one
 * designated for mmap operations. This region has changed over kernel
 * iterations and sometimes it can be informative to know what the dimensions
 * of this region are on your current kernel. This code determines the bounds
 * of this region.
 *
 * Note that it assumes your mmap region is contiguous and I would not
 * recommend running it on a 64-bit kernel unless you want to wait a long
 * while.
 */

#include <sys/mman.h>
#include <stdio.h>
#include <stdint.h>

int main(int argc, char **argv) {
    uintptr_t low = 0, high = 0, current;

    /* Mmap byte-by-byte til we can't map anymore, tracking the lowest and
     * highest seen addresses.
     */
    while ((current = (uintptr_t)mmap(NULL, 1, PROT_READ,
            MAP_PRIVATE|MAP_ANONYMOUS, -1, /* ignored */ 0))
            != (uintptr_t)MAP_FAILED) {
        if (low == 0 || current < low) {
            low = current;
        }
        if (high == 0 || current + 1 > high) {
            high = current + 1;
        }
    }
    printf("mmap region is %p - %p\n", (void*)low, (void*)high);
    return 0;
}
