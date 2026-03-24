/// @file
/// @brief Test multiple threads using the set.h API
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <assert.h>
#include <stddef.h>
#include <ute/set.h>

typedef SET(int) ints_t;

typedef struct {
  ints_t *ints;
  size_t thread_id;
} state_t;

static THREAD_RET entry(void *arg) {
  assert(arg != NULL);
  state_t *const s = arg;

  for (int i = 0; i < 10; ++i) {
    const int r = SET_INSERT(s->ints, i);
    ASSERT_EQ(r, 0);
    ASSERT_LE(SET_SIZE(s->ints), 10u);
  }

  for (int i = 0; i < 10; ++i) {
    (void)SET_REMOVE(s->ints, i);
    ASSERT_LE(SET_SIZE(s->ints), 10u);
  }

  return 0;
}

/// multiple threads doing the same operation
TEST("int set multithreaded, overlapping operations") {

  ints_t ints = {0};
  thread_t t[32];
  state_t s[sizeof(t) / sizeof(t[0])];

  // setup states
  for (size_t i = 0; i < sizeof(s) / sizeof(s[0]); ++i)
    s[i] = (state_t){.ints = &ints, .thread_id = i};

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

  ASSERT_EQ(SET_SIZE(&ints), 0u);
  SET_FREE(&ints);
}
