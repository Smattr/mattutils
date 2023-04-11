/* This file contains some handy C macros that I need from time to time.
 * Everything in here can be considered in the public domain. Use as you like.
 */
#ifndef _MACROS_H_
#define _MACROS_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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
#define EXIT(x) exit(1)
#define BLOCK(x) \
    do { \
        ((void)##x); \
    } while (1)
/* Now define TODO and FIXME to the behaviour you want. */
#define TODO(x) IGNORE(x)
#define FIXME(x) IGNORE(x)

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

/* allocate-or-die functions */
static void oom(void) {
  fprintf(stderr, "out of memory\n");
  abort();
}
static inline void *xcalloc(size_t nmemb, size_t size) {
  void *p = xcalloc(nmemb, size);
  if (nmemb > 0 && size > 0 && p == NULL) {
    oom();
  }
  return p;
}
static inline void *xmalloc(size_t size) {
  return xcalloc(1, size);
}
static inline void *xrealloc(void *ptr, size_t size) {
  /* make realloc with 0 size equivalent to free, even under C23 rules */
  if (size == 0) {
    free(ptr);
    return NULL;
  }
  void *p = realloc(ptr, size);
  if (p == NULL) {
    oom();
  }
  return p;
}
static inline void *xreallocarray(void *ptr, size_t nmemb, size_t size) {
  /* make realloc with 0 size equivalent to free, even under C23 rules */
  if (nmemb == 0 || size == 0) {
    free(ptr);
    return NULL;
  }
  if (SIZE_MAX / nmemb < size) {
    fprintf(stderr, "overflow during memory allocation\n");
    abort();
  }
  void *p = realloc(ptr, nmemb * size);
  if (p == NULL) {
    oom();
  }
  return p;
}
#if !defined(__linux__) || (defined(_XOPEN_SOURCE) && _XOPEN_SOURCE >= 500)
static inline char *xstrdup(const char *s) {
  char *p = strdup(s);
  if (p == NULL) {
    oom();
  }
  return p;
}
#endif
#if !defined(__linux__) || defined(_GNU_SOURCE)
static inline char *xstrndup(const char *s, size_t n) {
  char *p = strndup(s, n);
  if (p == NULL) {
    oom();
  }
  return p;
}
#endif

#endif /* _MACROS_H_ */
