/// @file
/// @brief Test races between `DICT_SET` and `DICT_CONTAINS`
///
/// In a previous dictionary design, it was possible for `DICT_CONTAINS` to
/// return false for a key that was present both before and after a `DICT_SET`
/// that is racing with it. The test case in this file probes whether this race
/// has been reintroduced.
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <ute/dict.h>

typedef DICT(int, int) ints_t;

typedef struct {
  size_t thread_id;
  ints_t *xs;
} state_t;

static THREAD_RET entry(void *arg) {
  assert(arg != NULL);
  state_t *const s = arg;

  // insert an entry we will then know to be present
  {
    const int r = DICT_SET(s->xs, 42, 1);
    ASSERT_EQ(r, 0);
  }

  // if we are the first thread, repeatedly update this entry
  if (s->thread_id == 0) {
    for (int i = 0; i < 100; ++i) {
      const int r = DICT_SET(s->xs, 42, i);
      ASSERT_EQ(r, 0);

      const bool present = DICT_CONTAINS(s->xs, 42);
      ASSERT(present);
    }
    return 0;
  }

  // otherwise, repeatedly check contains
  for (int i = 0; i < 100; ++i) {

    const bool present = DICT_CONTAINS(s->xs, 42);
    ASSERT(present);
  }

  return 0;
}

TEST("dict racing set and contains") {

  ints_t xs = {0};
  thread_t t[32];
  state_t s[sizeof(t) / sizeof(t[0])];

  // setup states
  for (size_t i = 0; i < sizeof(s) / sizeof(s[0]); ++i)
    s[i] = (state_t){.xs = &xs, .thread_id = i};

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

  DICT_FREE(&xs);
}
