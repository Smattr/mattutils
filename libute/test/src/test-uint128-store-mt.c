/// @file
/// @brief Test multi-threaded atomic writing of a 128-bit unsigned integer
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <assert.h>
#include <stdint.h>
#include <ute/int128.h>

typedef struct {
  uint128_t *target;
  uint128_t desired;
} state_t;

static THREAD_RET entry(void *arg) {
  assert(arg != NULL);
  state_t *const s = arg;

  // store the requested value
  uint128_atomic_store(s->target, s->desired);

  // read back the stored value
  const uint128_t actual = uint128_atomic_load(s->target);

  // This should be what we wrote, even under multi-threading as all threads are
  // trying to write the same value. That is, we should see no torn writes.
  ASSERT(actual == s->desired);

  return 0;
}

static void run(uint128_t *target, uint128_t desired) {
  assert(target != NULL);

  state_t s = {.target = target, .desired = desired};

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

TEST("uint128_atomic_store multi-threaded") {
  run(&(uint128_t){0}, 0);
  run(&(uint128_t){0}, 1);
  run(&(uint128_t){0}, 42);
  run(&(uint128_t){0}, (uint128_t)UINT64_MAX + 1);
}
