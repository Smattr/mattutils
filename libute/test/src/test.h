/// @file
/// @brief Basic unit testing framework
///
/// All content in this file is in the public domain. Use it any way you wish.

#pragma once

#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct test_case_ {
  void (*function)(void);
  const char *description;
  struct test_case_ *next;
} test_case_t;

extern test_case_t *test_cases;

#define JOIN_(x, y) x##y
#define JOIN(x, y) JOIN_(x, y)

extern atomic_flag has_assertion_;

/// an action to be run on (success or fail) exit in a test case
typedef struct cleanup_ {
  void (*function)(void *arg);
  void *arg;
  struct cleanup_ *next;
} cleanup_t;

/// registered cleanup actions of the current test case
extern cleanup_t *cleanups;

/// register a cleanup action to run on exit
void register_cleanup(void (*function)(void *arg), void *arg);

/// run and deregister all cleanup actions
void run_cleanups(void);

#define TEST(desc)                                                             \
  static void JOIN(test_, __LINE__)(void);                                     \
  static void __attribute__((constructor)) JOIN(add_test_, __LINE__)(void) {   \
    static test_case_t JOIN(test_case_, __LINE__) = {                          \
        .function = JOIN(test_, __LINE__),                                     \
        .description = desc,                                                   \
    };                                                                         \
    for (test_case_t **t = &test_cases;; t = &(*t)->next) {                    \
      if (*t == NULL || strcmp((desc), (*t)->description) < 0) {               \
        JOIN(test_case_, __LINE__).next = *t;                                  \
        *t = &JOIN(test_case_, __LINE__);                                      \
        return;                                                                \
      }                                                                        \
      if (strcmp((desc), (*t)->description) == 0) {                            \
        fprintf(stderr, "duplicate test cases \"%s\"\n", (desc));              \
        fflush(stderr);                                                        \
        abort();                                                               \
      }                                                                        \
    }                                                                          \
    __builtin_unreachable();                                                   \
  }                                                                            \
  static void JOIN(test_, __LINE__)(void)

#define PRINT_FMT(x)                                                           \
  _Generic((x),                                                                \
      char: "%c",                                                              \
      signed char: "%hhd",                                                     \
      unsigned char: "%hhu",                                                   \
      int: "%d",                                                               \
      long: "%ld",                                                             \
      long long: "%lld",                                                       \
      unsigned: "%u",                                                          \
      unsigned long: "%lu",                                                    \
      unsigned long long: "%llu",                                              \
      void *: "%p",                                                            \
      const void *: "%p")

#define ASSERT_(a, a_name, op, b, b_name)                                      \
  do {                                                                         \
    atomic_flag_test_and_set_explicit(&has_assertion_, memory_order_release);  \
    __typeof__(a) a_ = (a);                                                    \
    __typeof__(b) b_ = (b);                                                    \
    if (!(a_ op b_)) {                                                         \
      flockfile(stderr);                                                       \
      fprintf(stderr, "failed\n    %s:%d: assertion “%s %s %s” failed\n",      \
              __FILE__, __LINE__, #a_name, #op, #b_name);                      \
      fprintf(stderr, "      %s = ", #a_name);                                 \
      fprintf(stderr, PRINT_FMT(a_), a_);                                      \
      fprintf(stderr, "\n");                                                   \
      fprintf(stderr, "      %s = ", #b_name);                                 \
      fprintf(stderr, PRINT_FMT(b_), b_);                                      \
      fprintf(stderr, "\n");                                                   \
      fflush(stderr);                                                          \
      funlockfile(stderr);                                                     \
      run_cleanups();                                                          \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#define ASSERT_EQ(a, b) ASSERT_(a, a, ==, b, b)
#define ASSERT_GE(a, b) ASSERT_(a, a, >=, b, b)
#define ASSERT_GT(a, b) ASSERT_(a, a, >, b, b)
#define ASSERT_LE(a, b) ASSERT_(a, a, <=, b, b)
#define ASSERT_LT(a, b) ASSERT_(a, a, <, b, b)
#define ASSERT_NE(a, b) ASSERT_(a, a, !=, b, b)

#define ASSERT(expr)                                                           \
  do {                                                                         \
    atomic_flag_test_and_set_explicit(&has_assertion_, memory_order_release);  \
    if (!(expr)) {                                                             \
      fprintf(stderr, "failed\n    %s:%d: assertion “%s” failed\n", __FILE__,  \
              __LINE__, #expr);                                                \
      fflush(stderr);                                                          \
      run_cleanups();                                                          \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#define ASSERT_NOT_NULL(p)                                                     \
  do {                                                                         \
    const void *p_ = (p);                                                      \
    ASSERT_(p_, p, !=, (const void *)NULL, NULL);                              \
  } while (0)

#define ASSERT_NULL(p)                                                         \
  do {                                                                         \
    const void *p_ = (p);                                                      \
    ASSERT_(p_, p, ==, (const void *)NULL, NULL);                              \
  } while (0)

#define ASSERT_STREQ(a, b)                                                     \
  do {                                                                         \
    atomic_flag_test_and_set_explicit(&has_assertion_, memory_order_release);  \
    const char *a_ = (a);                                                      \
    const char *b_ = (b);                                                      \
    if (strcmp(a_, b_) != 0) {                                                 \
      flockfile(stderr);                                                       \
      fprintf(stderr,                                                          \
              "failed\n    %s:%d: assertion “strcmp(%s, %s) == 0” failed\n",   \
              __FILE__, __LINE__, #a, #b);                                     \
      fprintf(stderr, "      %s = \"%s\"\n", #a, a_);                          \
      fprintf(stderr, "      %s = \"%s\"\n", #b, b_);                          \
      fflush(stderr);                                                          \
      funlockfile(stderr);                                                     \
      run_cleanups();                                                          \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#define ASSERT_STRNE(a, b)                                                     \
  do {                                                                         \
    atomic_flag_test_and_set_explicit(&has_assertion_, memory_order_release);  \
    const char *a_ = (a);                                                      \
    const char *b_ = (b);                                                      \
    if (strcmp(a_, b_) == 0) {                                                 \
      flockfile(stderr);                                                       \
      fprintf(stderr,                                                          \
              "failed\n    %s:%d: assertion “strcmp(%s, %s) != 0” failed\n",   \
              __FILE__, __LINE__, #a, #b);                                     \
      fprintf(stderr, "      %s = \"%s\"\n", #a, a_);                          \
      fprintf(stderr, "      %s = \"%s\"\n", #b, b_);                          \
      fflush(stderr);                                                          \
      funlockfile(stderr);                                                     \
      run_cleanups();                                                          \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#define FAIL(...)                                                              \
  do {                                                                         \
    flockfile(stderr);                                                         \
    fprintf(stderr, "failed\n    ");                                           \
    fprintf(stderr, __VA_ARGS__);                                              \
    fflush(stderr);                                                            \
    funlockfile(stderr);                                                       \
    run_cleanups();                                                            \
    abort();                                                                   \
  } while (0)

////////////////////////////////////////////////////////////////////////////////
// multi-threading abstractions
////////////////////////////////////////////////////////////////////////////////

#if USE_PTHREADS
#include <pthread.h>

typedef pthread_t thread_t;

#define THREAD_CREATE(thread, start, arg)                                      \
  pthread_create((thread), NULL, (start), (arg))

#define THREAD_JOIN(thread, retval) pthread_join((thread), (retval))

/// return type of thread entry point
#define THREAD_RET void *

#else
#include <threads.h>

typedef thrd_t thread_t;

#define THREAD_CREATE(thread, start, arg)                                      \
  (thrd_create((thread), (start), (arg)) != thrd_success)

#define THREAD_JOIN(thread, retval)                                            \
  (thrd_join((thread), (retval)) != thrd_success)

/// return type of thread entry point
#define THREAD_RET int
#endif
