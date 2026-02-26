/// @file
/// @brief Test multi-threaded atomic reading of a 128-bit signed integer
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <ute/int128.h>

#if USE_PTHREADS
#include <pthread.h>
typedef pthread_t thread_t;
#define THREAD_CREATE(thread, start, arg)                                      \
  pthread_create((thread), NULL, (start), (arg))
#define THREAD_JOIN(thread, retval) pthread_join((thread), (retval))
#define THREAD_RET void *
#else
#include <threads.h>
typedef thrd_t thread_t;
#define THREAD_CREATE(thread, start, arg) thrd_create((thread), (start), (arg))
#define THREAD_JOIN(thread, retval) thrd_join((thread), (retval))
#define THREAD_RET int
#endif

typedef struct {
  int128_t *target;
  int128_t expected;
} state_t;

static THREAD_RET entry(void *arg) {
  state_t *const s = arg;

  const int128_t actual = int128_atomic_load(s->target);
  ASSERT(actual == s->expected);

  return 0;
}

static void run(int128_t *target) {
  assert(target != NULL);

  state_t s = {.target = target, .expected = *target};

  thread_t t[10];
  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    const int r = THREAD_CREATE(&t[i], entry, &s);
    ASSERT_EQ(r, 0);
  }

  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    THREAD_RET ret = 0;
    const int r = THREAD_JOIN(t[i], &ret);
    ASSERT_EQ(r, 0);
    ASSERT_EQ(ret, (THREAD_RET){0});
  }
}

TEST("int128_atomic_load multi-threaded") {
  run(&(int128_t){0});
  run(&(int128_t){-1});
  run(&(int128_t){42});
  run(&(int128_t){-42});
  run(&(int128_t){-1});
  run(&(int128_t){(int128_t)UINT64_MAX + 1});
}
