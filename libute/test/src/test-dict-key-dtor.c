/// @file
/// @brief Test dictionary key destructor is invoked properly
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ute/dict.h>

static void dtor(void *key) {
  assert(key != NULL);

  char **const k = key;
  free(*k);
}

TEST("dict with a key destructor") {
  DICT(char *, size_t) strs = {.key_dtor = dtor};

  const char *strings[] = {"foo", "bar", "qux", "quux", "corge"};

  // populate the dictionary with heap-allocated keys
  char *to_remove;
  for (size_t i = 0; i < sizeof(strings) / sizeof(strings[0]); ++i) {
    char *const k = strdup(strings[i]);
    ASSERT_NOT_NULL(k);
    const int r = DICT_SET(&strs, k, i);
    ASSERT_EQ(r, 0);
    if (i == 0)
      to_remove = k;
  }

  // overwrite one with a different heap-allocated key to confirm `DICT_SET`
  // calls the destructor
  {
    char *const k = strdup(strings[1]);
    ASSERT_NOT_NULL(k);
    const int r = DICT_SET(&strs, k, 1);
    ASSERT_EQ(r, 0);
  }

  // remove one to confirm `DICT_REMOVE` calls the destructor
  {
    const bool r = DICT_REMOVE(&strs, to_remove);
    ASSERT(r);
  }

  // confirm free calls the destructor on any remaining entries
  DICT_FREE(&strs);
}
