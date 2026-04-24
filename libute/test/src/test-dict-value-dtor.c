/// @file
/// @brief Test dictionary value destructor is invoked properly
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ute/dict.h>

static void dtor(void *value) {
  assert(value != NULL);

  char **const v = value;
  free(*v);
}

TEST("dict with a value destructor") {
  DICT(size_t, char *) strs = {.value_dtor = dtor};

  const char *strings[] = {"foo", "bar", "qux", "quux", "corge"};

  // populate the dictionary with heap-allocated values
  for (size_t i = 0; i < sizeof(strings) / sizeof(strings[0]); ++i) {
    char *const v = strdup(strings[i]);
    ASSERT_NOT_NULL(v);
    const int r = DICT_SET(&strs, i, v);
    ASSERT_EQ(r, 0);
  }

  // overwrite one with a different heap-allocated value to confirm `DICT_SET`
  // calls the destructor
  {
    char *const v = strdup(strings[1]);
    ASSERT_NOT_NULL(v);
    const int r = DICT_SET(&strs, 1, v);
    ASSERT_EQ(r, 0);
  }

  // remove one to confirm `DICT_REMOVE` calls the destructor
  {
    const bool r = DICT_REMOVE(&strs, 2);
    ASSERT(r);
  }

  // confirm free calls the destructor on any remaining entries
  DICT_FREE(&strs);
}
