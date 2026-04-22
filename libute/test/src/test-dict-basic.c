/// @file
/// @brief Test cases for basic dictionary functionality
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <stdbool.h>
#include <stddef.h>
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
