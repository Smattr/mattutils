/// @file
/// @brief Test multiple threads using the dict.h API
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <ute/dict.h>

typedef DICT(int, int) ints_t;

typedef struct {
  ints_t *xs;
  size_t thread_id;
  bool remove; ///< remove elements that are added?
} state_t;

/// construct a unique value based on thread ID
static int make_val(int thread_id, int i) { return (thread_id << 8) | i; }

static THREAD_RET entry(void *arg) {
  assert(arg != NULL);
  state_t *const s = arg;

  for (int i = 0; i < 10; ++i) {
    const int k = make_val((int)s->thread_id, i);
    const int r = DICT_SET(s->xs, k, k + 1);
    ASSERT_EQ(r, 0);

    for (int j = 0; j < 10; ++j) {
      const int k2 = make_val((int)s->thread_id, j);
      const bool present = DICT_CONTAINS(s->xs, k2);
      if (j <= i) {
        ASSERT(present);
      } else {
        ASSERT(!present);
      }
    }
  }

  for (int i = 0; s->remove && i < 10; ++i) {
    const int k = make_val((int)s->thread_id, i);
    const bool r = DICT_REMOVE(s->xs, k);
    ASSERT(r);

    for (int j = 0; j < 10; ++j) {
      const int k2 = make_val((int)s->thread_id, j);
      const bool present = DICT_CONTAINS(s->xs, k2);
      if (j <= i) {
        ASSERT(!present);
      } else {
        ASSERT(present);
      }
    }
  }

  return 0;
}

/// some operations on an int→int dictionary from multiple threads
TEST("int→int dict multithreaded") {

  ints_t xs = {0};
  thread_t t[16];
  state_t s[sizeof(t) / sizeof(t[0])];

  // setup states
  for (size_t i = 0; i < sizeof(s) / sizeof(s[0]); ++i)
    s[i] = (state_t){.xs = &xs, .thread_id = i, .remove = true};

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

  ASSERT_EQ(DICT_SIZE(&xs), 0u);
  DICT_FREE(&xs);
}

/// some operations on an int→int dictionary from multiple threads, no clean up
TEST("int→int dict multithreaded, leave set non-empty") {

  ints_t xs = {0};
  thread_t t[16];
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
