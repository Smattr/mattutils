/// @file
/// @brief Test multiple threads doing conflicting ops using the dict.h API
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <assert.h>
#include <stddef.h>
#include <ute/dict.h>

typedef DICT(int, int) ints_t;

typedef struct {
  ints_t *xs;
  size_t thread_id;
  size_t n_threads;
} state_t;

static THREAD_RET entry(void *arg) {
  assert(arg != NULL);
  state_t *const s = arg;

  for (int i = 0; i < 10; ++i) {
    const int r = DICT_SET(s->xs, i, i + 1);
    ASSERT_EQ(r, 0);
    ASSERT_LE(DICT_SIZE(s->xs), 10 + s->n_threads - 1);
  }

  for (int i = 0; i < 10; ++i) {
    (void)DICT_REMOVE(s->xs, i);
    ASSERT_LE(DICT_SIZE(s->xs), 10 + s->n_threads - 1);
  }

  return 0;
}

/// multiple threads doing the same operation
TEST("int→int dict, overlapping operations") {

  ints_t xs = {0};
  thread_t t[32];
  state_t s[sizeof(t) / sizeof(t[0])];

  // setup states
  for (size_t i = 0; i < sizeof(s) / sizeof(s[0]); ++i)
    s[i] = (state_t){
        .xs = &xs, .thread_id = i, .n_threads = sizeof(t) / sizeof(t[0])};

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
