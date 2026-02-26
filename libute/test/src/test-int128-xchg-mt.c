/// @file
/// @brief Test multi-threaded atomic exchange of a 128-bit signed integer
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <ute/int128.h>

typedef struct {
  size_t thread_id;
  size_t n_threads;
  int128_t *target;
} state_t;

static THREAD_RET entry(void *arg) {
  assert(arg != NULL);
  state_t *const s = arg;

  // derive an artbitrary different value to the initial one based on our thread
  // ID
  const int128_t desired = s->thread_id + 1;

  // exchange this with the current value
  const int128_t old = int128_atomic_xchg(s->target, desired);

  // we should have seen either the initial value or something written by a
  // thread that is not ourselves
  const bool ok = old >= 0 && old != s->thread_id + 1 && old <= s->n_threads;
  ASSERT(ok);

  return 0;
}

TEST("int128_atomic_xchg multi-threaded") {

  thread_t t[100];
  state_t s[sizeof(t) / sizeof(t[0])];

  int128_t target = 0;

  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    s[i] = (state_t){.thread_id = i,
                     .n_threads = sizeof(t) / sizeof(t[0]),
                     .target = &target};
    const int r = THREAD_CREATE(&t[i], entry, &s[i]);
    ASSERT_EQ(r, 0);
  }

  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    THREAD_RET ret = 0;
    const int r = THREAD_JOIN(t[i], &ret);
    ASSERT_EQ(r, 0);
    ASSERT_EQ(ret, (THREAD_RET){0});
  }
}
