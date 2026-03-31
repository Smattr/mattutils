/// @file
/// @brief Test user-supplied destructors to set API
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <stdlib.h>
#include <string.h>
#include <ute/set.h>

struct foo {
  char *payload;
};

static void dtor(void *foo) {
  struct foo *const f = foo;
  free(f->payload);
}

/// does `SET_REMOVE` correctly invoke user destructors?
TEST("set with user destructor (remove)") {
  SET(struct foo, dtor) foos = {.dtor = {dtor}};

  struct foo fs[10] = {0};
  for (size_t i = 0; i < sizeof(fs) / sizeof(fs[0]); ++i) {
    fs[i] = (struct foo){.payload = strdup((char[]){'a' + (char)i, '\0'})};
    ASSERT_NOT_NULL(fs[i].payload);
    const int r = SET_INSERT(&foos, fs[i]);
    ASSERT_EQ(r, 0);
    ASSERT_EQ(SET_SIZE(&foos), i + 1);
  }

  for (size_t i = 0; i < sizeof(fs) / sizeof(fs[0]); ++i) {
    const bool r = SET_REMOVE(&foos, fs[i]);
    ASSERT(r);
    ASSERT_EQ(SET_SIZE(&foos), 10 - i - 1);
  }

  SET_FREE(&foos);
}

/// does `SET_FREE` correctly invoke user destructors?
TEST("set with user destructor (free)") {
  SET(struct foo, dtor) foos = {.dtor = {dtor}};

  struct foo fs[10] = {0};
  for (size_t i = 0; i < sizeof(fs) / sizeof(fs[0]); ++i) {
    fs[i] = (struct foo){.payload = strdup((char[]){'a' + (char)i, '\0'})};
    ASSERT_NOT_NULL(fs[i].payload);
    const int r = SET_INSERT(&foos, fs[i]);
    ASSERT_EQ(r, 0);
    ASSERT_EQ(SET_SIZE(&foos), i + 1);
  }

  SET_FREE(&foos);
}
