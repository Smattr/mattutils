/// @file
/// @brief Test of set.h API with larger alignments
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <ute/set.h>

/// a type with a larger alignment than `malloc` normally supports
struct wide {
  alignas(128) int x;
};

/// construct a `wide` value
static struct wide make_wide(int x) {
  // FIXME: we need to initialise this struct via `memset` instead of braced
  // initialisation because we need to zero its padding too. This is because the
  // set implementation currently always compares with `memcmp`. Rephrase this
  // to use a custom comparator when this is supported.
  struct wide ret;
  memset(&ret, 0, sizeof(ret));
  ret.x = x;
  return ret;
}

/// basic operations on a set storing values with large alignment
TEST("set with elements with large alignment") {
  SET(struct wide) wides = {0};

  for (int i = 0; i < 10; ++i) {
    const int r = SET_INSERT(&wides, make_wide(i));
    ASSERT_EQ(r, 0);
    ASSERT_EQ(SET_SIZE(&wides), (size_t)i + 1);

    for (int j = 0; j < 10; ++j) {
      const bool present = SET_CONTAINS(&wides, make_wide(j));
      if (j <= i) {
        ASSERT(present);
      } else {
        ASSERT(!present);
      }
    }
  }

  for (int i = 0; i < 10; ++i) {
    const bool r = SET_REMOVE(&wides, make_wide(i));
    ASSERT(r);
    ASSERT_EQ(SET_SIZE(&wides), 10 - (size_t)i - 1);

    for (int j = 0; j < 10; ++j) {
      const bool present = SET_CONTAINS(&wides, make_wide(j));
      if (j <= i) {
        ASSERT(!present);
      } else {
        ASSERT(present);
      }
    }
  }

  SET_FREE(&wides);
}
