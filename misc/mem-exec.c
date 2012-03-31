/* This code tests the executability of your heap or stack. Depending on your
 * kernel and/or architecture, your heap/stack may be executable or not
 * (depending on NX support) and it is often desirable to know which situation
 * you are in.
 */

#include <assert.h>
#include <alloca.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The region we're trying to execute. */
char *region;

/* A signal handler we will make use of later. */
static void handler(int sig) {
    printf("%s is not executable.\n", region);
    exit(1);
}

/* Now we will define a test function that we will inject into the region we
 * want to test. The following defines the function "int test(){return 1;}". It
 * is slightly annoying to have to define this in assembly - partly because it
 * is now x86 only and partly because assembly is just yuck - but when trying
 * to define this in C with a trailing label test_end (that we need to
 * calculate the size of the function), I found GCC always dropped this label
 * as dead code regardless of what volatile and other magic I added around it.
 *
 * Also, while we're on the subject, whose bright idea was it to make inline
 * assembly syntax for referencing registers %%reg for extended asm and %reg
 * for constrained asm?
 */
extern int test(void);
extern unsigned char test_end[1];
asm ("\n"
    "test:\n"
    "\tpushl %ebp\n"
    "\tmovl  $1, %eax\n"
    "\tmovl  %esp, %ebp\n"
    "\tpopl  %ebp\n"
    "\tret\n"
    "test_end:\n"
);

int main(int argc, char **argv) {
    void *func;
    /* The size of the test function. */
    size_t test_size = (size_t)((uintptr_t)&test_end - (uintptr_t)&test);

    /* Check command line arguments. */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s area\n"
                        " Test an area of memory for executability.\n"
                        " area - \"heap\" or \"stack\"\n", argv[0]);
        return -1;
    }
    if (strcmp(argv[1], "heap") && strcmp(argv[1], "stack")) {
        fprintf(stderr, "Invalid area.\n");
        return -1;
    }

    /* Save the region we're checking so the signal handler can reference it.
     */
    region = argv[1];

    /* Install a signal handler to catch if the memory we're trying to execute
     * is non-executable.
     */
    (void)signal(SIGSEGV, &handler);
#ifdef SIGBUS
    (void)signal(SIGBUS, &handler);
#endif

    /* Allocate some space in the target region. */
    if (!strcmp(region, "heap")) {
        func = malloc(test_size);
    } else if (!strcmp(region, "stack")) {
        func = alloca(test_size);
    } else {
        /* Checks previously should ensure we never end up here. */
        assert(!"Region not heap or stack");
    }

    if (func == NULL) {
        fprintf(stderr, "Allocation failed.\n");
        return -1;
    }

    /* Inject the test function. */
    (void)memcpy(func, (void*)&test, test_size);

    /* Invoke the injected function. */
    if (((int(*)(void))func)()) {
        printf("%s is executable.\n", region);
        return 0;
    } else {
        /* Slightly weird. We invoked the function and apparently executed some
         * other code.
         */
        fprintf(stderr, "%s is not executable.\n", region);
        return -1;
    }
}
