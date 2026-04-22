/// @file
/// @brief Test set interface with packed types
///
/// Packed types can involve unusual sizes and alignments. Given the set
/// implementation can rely on these kind of properties, the tests below check
/// we can deal with these unusual cases.
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <stdint.h>
#include <ute/set.h>

TEST("packed struct set, int") {
  struct foo {
    int x;
    char y;
  } __attribute__((packed));

  SET(struct foo) xs = {0};

  for (char i = 0; i < 10; ++i) {
    const int r = SET_INSERT(&xs, ((struct foo){.x = i, .y = i}));
    ASSERT_EQ(r, 0);
    ASSERT_EQ(SET_SIZE(&xs), i + 1u);
  }

  SET_FREE(&xs);
}

TEST("packed struct set, uint64_t") {
  struct foo {
    uint64_t x;
    char y;
  } __attribute__((packed));

  SET(struct foo) xs = {0};

  for (char i = 0; i < 10; ++i) {
    const int r = SET_INSERT(&xs, ((struct foo){.x = i, .y = i}));
    ASSERT_EQ(r, 0);
    ASSERT_EQ(SET_SIZE(&xs), i + 1u);
  }

  SET_FREE(&xs);
}

#ifdef __SIZEOF_INT128__
TEST("packed struct set, uint128_t") {
  struct foo {
    unsigned __int128 x;
    char y;
  } __attribute__((packed));

  SET(struct foo) xs = {0};

  for (char i = 0; i < 10; ++i) {
    const int r = SET_INSERT(&xs, ((struct foo){.x = i, .y = i}));
    ASSERT_EQ(r, 0);
    ASSERT_EQ(SET_SIZE(&xs), i + 1u);
  }

  SET_FREE(&xs);
}
#endif
