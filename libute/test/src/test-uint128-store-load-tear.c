/// @file
/// @brief Count up in a CAS loop, but using store+load
///
/// This test case is designed to provoke torn reads and writes if they are
/// possible with stores to and loads from 128-bit variables that are supposed
/// to be atomic.
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <ute/int128.h>

typedef struct {
  size_t thread_id;
  uint128_t *target;
} state_t;

static THREAD_RET entry(void *arg) {
  assert(arg != NULL);
  state_t *const s = arg;

  while (true) {
    // load the current counter value
    const uint128_t old = uint128_atomic_load(s->target);

    {
      size_t nonzero_bits = 0;
      for (size_t i = 0; i < sizeof(old) * CHAR_BIT; ++i)
        nonzero_bits += (old >> i) & 1;
      ASSERT_LE(nonzero_bits, CHAR_BIT); // no torn reads/writes
    }

    // what we are waiting for is our ID + 1 shifted by some amount
    const uint128_t id = s->thread_id + 1;
    assert(id < UINT8_MAX);
    const uint128_t expected =
        id << (s->thread_id % (sizeof(uint128_t) * CHAR_BIT));
    if (old != expected)
      continue;

    // write the value the next thread will expect
    const uint128_t next = s->thread_id + 2;
    assert(next < UINT8_MAX);
    const uint128_t desired =
        next << ((s->thread_id + 1) % (sizeof(uint128_t) * CHAR_BIT));
    {
      size_t nonzero_bits = 0;
      for (size_t i = 0; i < sizeof(desired) * CHAR_BIT; ++i)
        nonzero_bits += (desired >> i) & 1;
      assert(nonzero_bits <= CHAR_BIT);
    }
    uint128_atomic_store(s->target, desired);

    break;
  }

  return 0;
}

TEST("128-bit CAS using load and store") {

  thread_t t[253];
  state_t s[sizeof(t) / sizeof(t[0])];

  uint128_t target = 1;

  // setup states first to avoid racing our reads of `target` with the test
  // case’s writes
  for (size_t i = 0; i < sizeof(s) / sizeof(s[0]); ++i)
    s[i] = (state_t){.thread_id = i, .target = &target};

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
