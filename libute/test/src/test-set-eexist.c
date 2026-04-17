/// @file
/// @brief Do set insertion callers get an indication of item-already-exists?

#include "test.h"
#include <stdbool.h>
#include <ute/set.h>

TEST("boxed set, exists") {
  struct foo {
    int x[10];
  };
  SET(struct foo) xs = {0};

  {
    struct foo x = {{42, 42}};
    bool exists;
    const int rc = SET_INSERT(&xs, x, &exists);
    ASSERT_EQ(rc, 0);
    ASSERT(!exists);
  }

  {
    struct foo x = {{42, 42}};
    bool exists;
    const int rc = SET_INSERT(&xs, x, &exists);
    ASSERT_EQ(rc, 0);
    ASSERT(exists);
  }

  SET_FREE(&xs);
}

TEST("unboxed set, exists") {
  SET(int) xs = {0};

  {
    bool exists;
    const int rc = SET_INSERT(&xs, 42, &exists);
    ASSERT_EQ(rc, 0);
    ASSERT(!exists);
  }

  {
    bool exists;
    const int rc = SET_INSERT(&xs, 42, &exists);
    ASSERT_EQ(rc, 0);
    ASSERT(exists);
  }

  SET_FREE(&xs);
}

TEST("bitset set, exists") {
  SET(short) xs = {0};

  {
    bool exists;
    const int rc = SET_INSERT(&xs, 42, &exists);
    ASSERT_EQ(rc, 0);
    ASSERT(!exists);
  }

  {
    bool exists;
    const int rc = SET_INSERT(&xs, 42, &exists);
    ASSERT_EQ(rc, 0);
    ASSERT(exists);
  }

  SET_FREE(&xs);
}

TEST("inline set, exists") {
  SET(bool) xs = {0};

  {
    bool exists;
    const int rc = SET_INSERT(&xs, true, &exists);
    ASSERT_EQ(rc, 0);
    ASSERT(!exists);
  }

  {
    bool exists;
    const int rc = SET_INSERT(&xs, true, &exists);
    ASSERT_EQ(rc, 0);
    ASSERT(exists);
  }

  SET_FREE(&xs);
}
