/// @file
/// @brief Test multi-threaded atomic reading of a 128-bit signed integer
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <ute/int128.h>

typedef struct {
  int128_t *target;
  int128_t expected;
} state_t;

static THREAD_RET entry(void *arg) {
  assert(arg != NULL);
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
  run(&(int128_t){1});
  run(&(int128_t){42});
  run(&(int128_t){-42});
  run(&(int128_t){-1});
  run(&(int128_t){(int128_t)UINT64_MAX + 1});
}
