/// @file
/// @brief Test multi-threaded failing atomic CAS of a 128-bit signed integer
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

  // derive an arbitrary different value to the initial one based on our thread
  // ID
  int128_t expected = s->thread_id;

  // derive another as our desired update
  const int128_t desired = s->thread_id + 1;

  // CAS should fail because we know it differs from the initial value
  const bool r = int128_atomic_cas(s->target, &expected, desired);
  ASSERT(!r);

  // our expectation should have been updated with the old value
  ASSERT(expected == s->init_val);

  // the stored value should not have been modified
  const int128_t stored = int128_atomic_load(s->target);
  ASSERT(stored == s->init_val);

  return 0;
}

TEST("int128_atomic_cas fail multi-threaded") {

  thread_t t[100];
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
