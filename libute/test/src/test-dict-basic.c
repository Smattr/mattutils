/// @file
/// @brief Test cases for basic dictionary functionality
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <ute/dict.h>

/// freeing an empty dictionary should be OK and should not leak memory
TEST("empty dict lifecycle") {
  DICT(int, int) ints = {0};
  ASSERT_EQ(DICT_SIZE(&ints), 0u);
  DICT_FREE(&ints);
}

/// some basic operations on an integer → integer set
TEST("int→int dict") {
  DICT(int, int) ints = {0};

  for (int i = 0; i < 10; ++i) {
    const int r = DICT_SET(&ints, i, i + 1);
    ASSERT_EQ(r, 0);
    ASSERT_EQ(DICT_SIZE(&ints), (size_t)i + 1);

    for (int j = 0; j < 10; ++j) {
      const bool present = DICT_CONTAINS(&ints, j);
      const int *const v = DICT_GET(&ints, j);
      if (j <= i) {
        ASSERT(present);
        ASSERT_NOT_NULL(v);
        ASSERT_EQ(*v, j + 1);
      } else {
        ASSERT(!present);
        ASSERT_NULL(v);
      }
    }
  }

  for (int i = 0; i < 10; ++i) {
    const bool r = DICT_REMOVE(&ints, i);
    ASSERT(r);
    ASSERT_EQ(DICT_SIZE(&ints), 10 - (size_t)i - 1);

    for (int j = 0; j < 10; ++j) {
      const bool present = DICT_CONTAINS(&ints, j);
      const int *const v = DICT_GET(&ints, j);
      if (j <= i) {
        ASSERT(!present);
        ASSERT_NULL(v);
      } else {
        ASSERT(present);
        ASSERT_NOT_NULL(v);
        ASSERT_EQ(*v, j + 1);
      }
    }
  }

  DICT_FREE(&ints);
}

/// a wider type to an object type
TEST("int64_t→struct dict") {
  struct foo {
    int x;
    long y;
    char z;
  };

  DICT(int64_t, struct foo) xs = {0};

  for (int i = 0; i < 10; ++i) {
    const int r = DICT_SET(&xs, i, ((struct foo){.x = i, .y = i}));
    ASSERT_EQ(r, 0);
    ASSERT_EQ(DICT_SIZE(&xs), (size_t)i + 1);

    for (int j = 0; j < 10; ++j) {
      const bool present = DICT_CONTAINS(&xs, j);
      const struct foo *const v = DICT_GET(&xs, j);
      if (j <= i) {
        ASSERT(present);
        ASSERT_NOT_NULL(v);
        ASSERT_EQ(v->x, j);
        ASSERT_EQ(v->y, j);
        ASSERT_EQ(v->z, 0);
      } else {
        ASSERT(!present);
        ASSERT_NULL(v);
      }
    }
  }

  for (int i = 0; i < 10; ++i) {
    const bool r = DICT_REMOVE(&xs, i);
    ASSERT(r);
    ASSERT_EQ(DICT_SIZE(&xs), 10 - (size_t)i - 1);

    for (int j = 0; j < 10; ++j) {
      const bool present = DICT_CONTAINS(&xs, j);
      const struct foo *const v = DICT_GET(&xs, j);
      if (j <= i) {
        ASSERT(!present);
        ASSERT_NULL(v);
      } else {
        ASSERT(present);
        ASSERT_NOT_NULL(v);
        ASSERT_EQ(v->x, j);
        ASSERT_EQ(v->y, j);
        ASSERT_EQ(v->z, 0);
      }
    }
  }

  DICT_FREE(&xs);
}

/// a dictionary with 0-sized key, a Clang/GCC extension
TEST("0-sized dictionary key") {
  struct foo {};
  DICT(struct foo, int) xs = {0};

  for (int i = 0; i < 10; ++i) {
    const int r = DICT_SET(&xs, (struct foo){}, 42);
    ASSERT_EQ(r, 0);
    ASSERT_EQ(DICT_SIZE(&xs), 1u);

    for (int j = 0; j < 10; ++j) {
      const bool present = DICT_CONTAINS(&xs, (struct foo){});
      ASSERT(present);
      const int *const v = DICT_GET(&xs, (struct foo){});
      ASSERT_NOT_NULL(v);
      ASSERT_EQ(*v, 42);
    }
  }

  for (int i = 0; i < 10; ++i) {
    const bool r = DICT_REMOVE(&xs, (struct foo){});
    if (i == 0) {
      ASSERT(r);
    } else {
      ASSERT(!r);
    }
    ASSERT_EQ(DICT_SIZE(&xs), 0u);

    const bool present = DICT_CONTAINS(&xs, (struct foo){});
    ASSERT(!present);
  }

  DICT_FREE(&xs);
}
