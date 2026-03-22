/// @file
/// @brief Test multiple threads using the set.h API
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <ute/set.h>

typedef SET(int) ints_t;

typedef struct {
  ints_t *ints;
  size_t thread_id;
  bool remove; ///< remove elements that are added?
} state_t;

/// construct a unique value based on thread ID
static int make_val(int thread_id, int i) { return (thread_id << 8) | i; }

static THREAD_RET entry(void *arg) {
  assert(arg != NULL);
  state_t *const s = arg;

  for (int i = 0; i < 10; ++i) {
    const int v = make_val((int)s->thread_id, i);
    const int r = SET_INSERT(s->ints, v);
    ASSERT_EQ(r, 0);
    ASSERT_GE(SET_SIZE(s->ints), (size_t)i + 1);

    for (int j = 0; j < 10; ++j) {
      const int v2 = make_val((int)s->thread_id, j);
      const bool present = SET_CONTAINS(s->ints, v2);
      if (j <= i) {
        ASSERT(present);
      } else {
        ASSERT(!present);
      }
    }
  }

  for (int i = 0; s->remove && i < 10; ++i) {
    const int v = make_val((int)s->thread_id, i);
    const bool r = SET_REMOVE(s->ints, v);
    ASSERT(r);
    ASSERT_GE(SET_SIZE(s->ints), 10 - (size_t)i - 1);

    for (int j = 0; j < 10; ++j) {
      const int v2 = make_val((int)s->thread_id, j);
      const bool present = SET_CONTAINS(s->ints, v2);
      if (j <= i) {
        ASSERT(!present);
      } else {
        ASSERT(present);
      }
    }
  }

  return 0;
}

/// some operations on an integer set from multiple threads
TEST("int set multithreaded") {

  ints_t ints = {0};
  thread_t t[32];
  state_t s[sizeof(t) / sizeof(t[0])];

  // setup states
  for (size_t i = 0; i < sizeof(s) / sizeof(s[0]); ++i)
    s[i] = (state_t){.ints = &ints, .thread_id = i, .remove = true};

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

/// some operations on an integer set from multiple threads, no clean up
TEST("int set multithreaded, leave set non-empty") {

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

  SET_FREE(&ints);
}
