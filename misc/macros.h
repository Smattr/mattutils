/* This file contains some handy C macros that I need from time to time.
 * Everything in here can be considered in the public domain. Use as you like.
 */
#ifndef _MACROS_H_
#define _MACROS_H_

/* When working with variable-arity functions and macros, you often want to
 * count the number of arguments being passed. This is surprisingly awkward and
 * the best technique I have found for accomplishing this is the following from
 * https://groups.google.com/group/comp.std.c/browse_thread/thread/77ee8c8f92e4a3fb/346fc464319b1ee5?pli=1.
 * The first three are supporting macros and should not be called directly;
 * just call NARGS with the list of arguments you need to inspect.
 */
#define SEQ_N 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
#define ARG_N(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...) N
#define NARG_(...) ARG_N(__VA_ARGS__)
#define NARGS(...) NARG_(, ## __VA_ARGS__, SEQ_N)

/* Sometimes you need to auto-generate unique symbols during compilation. The
 * natural way of doing this is with the __LINE__ macro, but joining it with
 * other characters to create a symbol is not straightforward. However, with an
 * extra level of indirection you can create pseudo-unique symbols. Use
 * JOIN(my_symbol, __LINE__) to produce such a symbol.
 */
#define _JOIN(x, y) x ## y
#define JOIN(x, y) _JOIN(x, y)

/* Compile-time assertions. There are many ways of implementing compile-time
 * assertions, but the best technique I've come across is the following, which
 * has the added advantages of being usable within macros that are expanded in
 * multiple places and also introduces no code overhead.
 */
#define compile_time_assert(name, condition) \
    typedef char JOIN(name##_, __LINE__)[(condition) ? 1 : -1 ]

/* General kernel-math helpers. */
#define BIT(n) (1<<(n))
#define MASK(n) (BIT(n) - 1)
#define IS(field, attribute) ((unsigned int)(field) & BIT(attribute))
#define SET(field, attribute) \
    ((field) = (typeof(field))((unsigned int)(field) | BIT(attribute)))
#define UNSET(field, attribute) \
    ((field) = (typeof(field))((unsigned int)(field) & ~BIT(attribute)))
#define ALIGNED(x, n) (((unsigned int)(x) & ((unsigned int)(n) - 1)) == 0)
#define ROUND_UP(x, n) \
    ((((unsigned int)(x) - 1) / (unsigned int)(n) + 1) * (unsigned int)(n))
#define ROUND_DOWN(x, n) \
    ((unsigned int)(x) / (unsigned int)(n) * (unsigned int)(n))

/* SQL-style syntax. Note that these both use their arguments more than once,
 * so don't pass expressions with side-effects.
 */
#define ISNULL(a, b) (((a) == NULL) ? (b) : (a))
#define NULLIF(a, b) (((a) == (b)) ? NULL : (a))

/* Some useful macros for stubbing out sections of code. You often want to mark
 * pieces of code as TODO or FIXME, but then later on forget to clean this up.
 * By making them macros, you can make them force a compile error (if you want
 * to aggressively weed out all instances), fail in a debug context, fail
 * always, exit or block execution. Seems simple, but in practice I've found
 * the ability to configure this behaviour invaluable.
 */
/* First macros for possible expansions: */
#define IGNORE(x) ((void)##x) /* Nothing */
#define COMPILE_ERROR(x) Forced compile time error: ##x
                                               /* Only works as long as other
                                                * macros don't interfere with
                                                * this expansion.
                                                */
#define DEBUG_FAIL(x) assert(!("Forced debug failure" ##x))
#define RUNTIME_FAIL(x) \
    do { \
        volatile int _a = 0; \
        _a = ((int)##x) / _a; /* Force division by 0. */ \
    } while (0)
#define EXIT(x) exit(1)
#define BLOCK(x) \
    do { \
        ((void)##x); \
    } while (1)
/* Now define TODO and FIXME to the behaviour you want. */
#define TODO(x) IGNORE(x)
#define FIXME(x) IGNORE(x)

/* When working on a simple project I wanted to use some nice syntax like the
 * following to indicate atomic sections:
 *
 *  atomic {
 *      // Do something stuff
 *  }
 *
 * Assuming you have the following functions for your lock:
 *  void lock(void);
 *  void unlock(void);
 *  int have_lock(void);
 * you can do something like the following:
 */
#define atomic for(;(!have_lock() && ({lock();1;})) || ({unlock();0;});)
/* It ain't pretty or well-tested, but it can make demonstrative code easier to
 * read.
 */

/* The ability of a compiler to optimise your code is constrained by what it
 * can determine about the code. If there's a particular fact that will help
 * the compiler, you can provide it with __builtin_unreachable. This macro
 * makes such code a bit clearer.
 */
#define ASSUME(fact) \
    do { \
        if (!(fact)) { \
            __builtin_unreachable(); \
        } \
    } while (0)

/* Evaluating a pre-processor symbol at compiler time. Clagged from
 * http://article.gmane.org/gmane.linux.kernel/1281138
 */
#define is_set(macro) is_set_(macro)
#define macrotest_1 ,
#define is_set_(value) is_set__(macrotest_##value)
#define is_set__(comma) is_set___(comma 1, 0)
#define is_set___(_, v, ...) v

#endif /* _MACROS_H_ */

