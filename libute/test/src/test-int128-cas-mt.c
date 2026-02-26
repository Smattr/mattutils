/// @file
/// @brief Test multi-threaded atomic CAS of a 128-bit signed integer
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
  int128_t init_val;
  int128_t *target;
} state_t;

static THREAD_RET entry(void *arg) {
  assert(arg != NULL);
  state_t *const s = arg;

  // if we are the first thread, swap our ID over the initial value
  if (s->thread_id == 0) {
    int128_t expected = s->init_val;
    const bool r = int128_atomic_cas(s->target, &expected, s->thread_id);
    // this should succeed because no other thread’s CAS should be able to
    // proceed before us
    ASSERT(r);
    ASSERT(expected == s->init_val);
    return 0;
  }

  // Try to swap in our ID, expecting the previous thread to write its ID first.
  // If we have made a mistake, the failure mode of this test case will usually
  // be for this to loop forever on some threads.
  while (true) {
    int128_t expected = s->thread_id - 1;
    if (int128_atomic_cas(s->target, &expected, s->thread_id))
      break;
  }

  return 0;
}

TEST("int128_atomic_cas multi-threaded") {

  thread_t t[16];
  state_t s[sizeof(t) / sizeof(t[0])];

  int128_t target = -1000;

  // setup states first to avoid racing our reads of `target` with the test
  // case’s writes
  for (size_t i = 0; i < sizeof(s) / sizeof(s[0]); ++i)
    s[i] = (state_t){.thread_id = i,
                     .n_threads = sizeof(t) / sizeof(t[0]),
                     .init_val = target,
                     .target = &target};

  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
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
