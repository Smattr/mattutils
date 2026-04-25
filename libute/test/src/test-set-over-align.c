/// @file
/// @brief Test of set.h API with larger alignments
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <ute/attr.h>
#include <ute/set.h>

/// a type with a larger alignment than `malloc` normally supports
struct wide {
  alignas(128) int x;
};

static size_t my_hash(const void *p, size_t size UNUSED) {
  const struct wide *const x = p;
  return (size_t)x->x;
}

static bool my_eq(const void *a, const void *b, size_t size UNUSED) {
  const struct wide *const x = a;
  const struct wide *const y = b;
  return x->x == y->x;
}

/// basic operations on a set storing values with large alignment
TEST("set with elements with large alignment") {
  // we need a custom hasher and comparator because the default ones look at the
  // padding bits
  SET(struct wide) wides = {.hash = my_hash, .eq = my_eq};

  for (int i = 0; i < 10; ++i) {
    const int r = SET_INSERT(&wides, (struct wide){i});
    ASSERT_EQ(r, 0);
    ASSERT_EQ(SET_SIZE(&wides), (size_t)i + 1);

    for (int j = 0; j < 10; ++j) {
      const bool present = SET_CONTAINS(&wides, (struct wide){j});
      if (j <= i) {
        ASSERT(present);
      } else {
        ASSERT(!present);
      }
    }
  }

  for (int i = 0; i < 10; ++i) {
    const bool r = SET_REMOVE(&wides, (struct wide){i});
    ASSERT(r);
    ASSERT_EQ(SET_SIZE(&wides), 10 - (size_t)i - 1);

    for (int j = 0; j < 10; ++j) {
      const bool present = SET_CONTAINS(&wides, (struct wide){j});
      if (j <= i) {
        ASSERT(!present);
      } else {
        ASSERT(present);
      }
    }
  }

  SET_FREE(&wides);
}
