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

/* Invoke the relevant version of bar based on the type of the argument. */
#define fn_bar_call(arg) \
    do { \
        typeof(arg) _a = 1.1; \
        if (_a != 1.1) { \
            /* The argument is an int. */ \
            fn_bar_int(arg); \
        } else { \
            /* The argument is a double. */ \
            fn_bar_double(arg); \
        } \
    } while (0)

/* Now we need to define bar such that it expands into the definition of
 * fn_bar_int, the definition of fn_bar_double or an invocation of either
 * depending on the context. I'm not even going to try to explain what's going
 * on below. If it doesn't make sense try running CPP over it several times
 * with different #defines commented out.
 *
 * [Note that there are several significant limitations to this technique. For
 * example, you can't invoke the function with a literal.]
 *
 * I picked up the _check_... trick via G+ user comex .
 */
#define __bar(original, _, arg, ...) JOIN(fn_bar_, arg)(original)
#define _bar(args...) __bar(args)
#define _check_int ,int,
#define _check_double ,double,
#define bar(arg) _bar(arg, _check_##arg, call)

void bar(int i) {
    printf("I am bar with an int parameter.\n");
}

void bar(double i) {
    printf("I am bar with a double parameter.\n");
}

void test_bar() {
    int i = 1;
    double j = 1.1;

    bar(i);
    bar(j);
}


/* Run some demonstrations of the implementations. */
int main(int argc, char **argv) {
    test_foo();
    test_bar();
    return 0;
}

