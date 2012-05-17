/* I wasn't sure whether it was possible to implement function overloading
 * syntax in C, so I had a go at doing this below. This is not how I advise
 * writing code. This was just one of those experiments to see if something was
 * possible for its own sake.
 *
 * Consider all code in this file in the public domain. However, I take no
 * responsibility for any problems you run into.
 *
 * Warning: This relies heavily on GCC extensions.
 */

#include <stdio.h>
#include <stdlib.h>
#include "macros.h"

/* First lets look at the easy case: overloading a function allowing different
 * numbers of arguments. You can already do this in C using varargs, but anyone
 * who has looked at a libc implementation will realise this comes with its own
 * drawbacks.
 */

#define foo(args...) JOIN(foo_, NARGS(args))(args)

void foo() {
    printf("I am foo with no arguments.\n");
}

void foo(int i) {
    printf("I am foo with 1 argument.\n");
}

void foo(int i, int j) {
    printf("I am foo with 2 arguments.\n");
}

void test_foo() {
    foo(1, 2);
    foo(4);
    foo();
}

/* Now something complicated and messy: overloading a function with a single
 * argument of different types.
 */

/* First we setup an array that we're going to use as a jump table indexed by
 * BAR_INT and BAR_DOUBLE.
 */
#define BAR_INT    0
#define BAR_DOUBLE 1
static void (*bar_call_table[2])(void*) = { NULL };

/* Invoke the relevant version of bar based on the type of the argument. */
#define call_bar(arg) \
    do { \
        typeof(arg) _a = 1.1; \
        if (_a != 1.1) { \
            /* The argument is an int. */ \
            ((void(*)(int))bar_call_table[BAR_INT])(arg); \
        } else { \
            /* The argument is a double. */ \
            ((void(*)(double))bar_call_table[BAR_DOUBLE])(arg); \
        } \
    } while (0)

/* Define the relevant version of bar. Note that this expands into two function
 * definitions. The easiest way to see what's going on may be to run CPP over
 * this file and check out the output.
 */
#define bar(arg) \
    JOIN(bar, __LINE__)(arg);                                                 \
        /* Function prototype that we're about to reference. */               \
                                                                              \
    void JOIN(_fill_bar_call_table, __LINE__)(void)                           \
        __attribute__((constructor));                                         \
        /* Make this function run when the program starts up. */              \
                                                                              \
    /* We're going to use this function to fill the bar_call_table with    */ \
    /* relevant function pointers.                                         */ \
    void JOIN(_fill_bar_call_table, __LINE__)(void) {                         \
        typedef arg, _type_alias; /* Ignore parameter name, but alias the  */ \
                                  /* type. May require staring at for a    */ \
                                  /* bit before it becomes clear.          */ \
        _type_alias _test_var = 1.1;                                          \
        if (_test_var != 1.1) {                                               \
            /* This is the int version. */                                    \
            bar_call_table[BAR_INT] = (void(*)(void*))&JOIN(bar, __LINE__);   \
        } else {                                                              \
            /* This is the double version. */                                 \
            bar_call_table[BAR_DOUBLE] = (void(*)(void*))&JOIN(bar, __LINE__);\
        }                                                                     \
    }                                                                         \
    void JOIN(bar, __LINE__)(arg) /* This should match up nicely with the  */ \
                                  /* { and body of the function.           */

void bar(int i) {
    printf("I am bar with an int parameter.\n");
}

void bar(double i) {
    printf("I am bar with a double parameter.\n");
}

/* FIXME: This is a bit of an irritation. If we could get away with no
 * pre-processor operations between the function definitions and calls all the
 * yuck could be abstracted into a header.
 */
#undef bar
#define bar call_bar

void test_bar() {
    bar(1);
    bar(1.1);
}


/* Run some demonstrations of the implementations. */
int main(int argc, char **argv) {
    test_foo();
    test_bar();
    return 0;
}

