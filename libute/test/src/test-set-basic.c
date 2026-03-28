/// @file
/// @brief Some tests of the generic set
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <stdbool.h>
#include <stdint.h>
#include <ute/set.h>

/// freeing an empty set should be OK and should not leak memory
TEST("empty set lifecycle") {
  SET(int) ints = {0};
  ASSERT_EQ(SET_SIZE(&ints), 0u);
  SET_FREE(&ints);
}

/// some basic operations on an boolean set
TEST("bool set") {
  SET(bool) bools = {0};

  for (int i = 0; i < 2; ++i) {
    const int r = SET_INSERT(&bools, i != 0);
    ASSERT_EQ(r, 0);
    ASSERT_EQ(SET_SIZE(&bools), (size_t)i + 1);

    for (int j = 0; j < 2; ++j) {
      const bool present = SET_CONTAINS(&bools, j != 0);
      if (j <= i) {
        ASSERT(present);
      } else {
        ASSERT(!present);
      }
    }
  }

  for (int i = 0; i < 2; ++i) {
    const bool r = SET_REMOVE(&bools, i != 0);
    ASSERT(r);
    ASSERT_EQ(SET_SIZE(&bools), 2 - (size_t)i - 1);

    for (int j = 0; j < 2; ++j) {
      const bool present = SET_CONTAINS(&bools, j != 0);
      if (j <= i) {
        ASSERT(!present);
      } else {
        ASSERT(present);
      }
    }
  }

  SET_FREE(&bools);
}

/// some basic operations on an character set
TEST("char set") {
  SET(char) chars = {0};

  for (int i = 0; i < 10; ++i) {
    const int r = SET_INSERT(&chars, 'a' + i);
    ASSERT_EQ(r, 0);
    ASSERT_EQ(SET_SIZE(&chars), (size_t)i + 1);

    for (int j = 0; j < 10; ++j) {
      const bool present = SET_CONTAINS(&chars, 'a' + j);
      if (j <= i) {
        ASSERT(present);
      } else {
        ASSERT(!present);
      }
    }
  }

  for (int i = 0; i < 10; ++i) {
    const bool r = SET_REMOVE(&chars, 'a' + i);
    ASSERT(r);
    ASSERT_EQ(SET_SIZE(&chars), 10 - (size_t)i - 1);

    for (int j = 0; j < 10; ++j) {
      const bool present = SET_CONTAINS(&chars, 'a' + j);
      if (j <= i) {
        ASSERT(!present);
      } else {
        ASSERT(present);
      }
    }
  }

  SET_FREE(&chars);
}

/// some basic operations on a short integer set
TEST("short set") {
  SET(short) shorts = {0};

  for (short i = 0; i < 10; ++i) {
    const int r = SET_INSERT(&shorts, i);
    ASSERT_EQ(r, 0);
    ASSERT_EQ(SET_SIZE(&shorts), (size_t)i + 1);

    for (short j = 0; j < 10; ++j) {
      const bool present = SET_CONTAINS(&shorts, j);
      if (j <= i) {
        ASSERT(present);
      } else {
        ASSERT(!present);
      }
    }
  }

  for (short i = 0; i < 10; ++i) {
    const bool r = SET_REMOVE(&shorts, i);
    ASSERT(r);
    ASSERT_EQ(SET_SIZE(&shorts), 10 - (size_t)i - 1);

    for (short j = 0; j < 10; ++j) {
      const bool present = SET_CONTAINS(&shorts, j);
      if (j <= i) {
        ASSERT(!present);
      } else {
        ASSERT(present);
      }
    }
  }

  SET_FREE(&shorts);
}

/// some basic operations on an integer set
TEST("int set") {
  SET(int) ints = {0};

  for (int i = 0; i < 10; ++i) {
    const int r = SET_INSERT(&ints, i);
    ASSERT_EQ(r, 0);
    ASSERT_EQ(SET_SIZE(&ints), (size_t)i + 1);

    for (int j = 0; j < 10; ++j) {
      const bool present = SET_CONTAINS(&ints, j);
      if (j <= i) {
        ASSERT(present);
      } else {
        ASSERT(!present);
      }
    }
  }

  for (int i = 0; i < 10; ++i) {
    const bool r = SET_REMOVE(&ints, i);
    ASSERT(r);
    ASSERT_EQ(SET_SIZE(&ints), 10 - (size_t)i - 1);

    for (int j = 0; j < 10; ++j) {
      const bool present = SET_CONTAINS(&ints, j);
      if (j <= i) {
        ASSERT(!present);
      } else {
        ASSERT(present);
      }
    }
  }

  SET_FREE(&ints);
}

/// some basic operations on a long integer set
TEST("long set") {
  SET(long) longs = {0};

  for (long i = 0; i < 10; ++i) {
    const int r = SET_INSERT(&longs, i);
    ASSERT_EQ(r, 0);
    ASSERT_EQ(SET_SIZE(&longs), (size_t)i + 1);

    for (long j = 0; j < 10; ++j) {
      const bool present = SET_CONTAINS(&longs, j);
      if (j <= i) {
        ASSERT(present);
      } else {
        ASSERT(!present);
      }
    }
  }

  for (long i = 0; i < 10; ++i) {
    const bool r = SET_REMOVE(&longs, i);
    ASSERT(r);
    ASSERT_EQ(SET_SIZE(&longs), 10 - (size_t)i - 1);

    for (long j = 0; j < 10; ++j) {
      const bool present = SET_CONTAINS(&longs, j);
      if (j <= i) {
        ASSERT(!present);
      } else {
        ASSERT(present);
      }
    }
  }

  SET_FREE(&longs);
}

/// some basic operations on a `uint64_t` set
TEST("uint64_t set") {
  SET(uint64_t) ints = {0};

  for (uint64_t i = 0; i < 10; ++i) {
    const int r = SET_INSERT(&ints, i);
    ASSERT_EQ(r, 0);
    ASSERT_EQ(SET_SIZE(&ints), (size_t)i + 1);

    for (uint64_t j = 0; j < 10; ++j) {
      const bool present = SET_CONTAINS(&ints, j);
      if (j <= i) {
        ASSERT(present);
      } else {
        ASSERT(!present);
      }
    }
  }

  for (uint64_t i = 0; i < 10; ++i) {
    const bool r = SET_REMOVE(&ints, i);
    ASSERT(r);
    ASSERT_EQ(SET_SIZE(&ints), 10 - (size_t)i - 1);

    for (uint64_t j = 0; j < 10; ++j) {
      const bool present = SET_CONTAINS(&ints, j);
      if (j <= i) {
        ASSERT(!present);
      } else {
        ASSERT(present);
      }
    }
  }

  SET_FREE(&ints);
}

/// some basic operations on an object set
TEST("aggregate set") {
  struct foo {
    int x;
  };
  SET(struct foo) ints = {0};

  for (int i = 0; i < 10; ++i) {
    const int r = SET_INSERT(&ints, (struct foo){.x = i});
    ASSERT_EQ(r, 0);
    ASSERT_EQ(SET_SIZE(&ints), (size_t)i + 1);

    for (int j = 0; j < 10; ++j) {
      const bool present = SET_CONTAINS(&ints, (struct foo){.x = j});
      if (j <= i) {
        ASSERT(present);
      } else {
        ASSERT(!present);
      }
    }
  }

  for (int i = 0; i < 10; ++i) {
    const bool r = SET_REMOVE(&ints, (struct foo){.x = i});
    ASSERT(r);
    ASSERT_EQ(SET_SIZE(&ints), 10 - (size_t)i - 1);

    for (int j = 0; j < 10; ++j) {
      const bool present = SET_CONTAINS(&ints, (struct foo){.x = j});
      if (j <= i) {
        ASSERT(!present);
      } else {
        ASSERT(present);
      }
    }
  }

  SET_FREE(&ints);
}

/// a set of 0-sized element type, a Clang/GCC extension
TEST("0-sized element set") {
  struct foo {};
  SET(struct foo) foos = {0};

  for (int i = 0; i < 10; ++i) {
    const int r = SET_INSERT(&foos, (struct foo){});
    ASSERT_EQ(r, 0);
    ASSERT_EQ(SET_SIZE(&foos), 1u);

    for (int j = 0; j < 10; ++j) {
      const bool present = SET_CONTAINS(&foos, (struct foo){});
      ASSERT(present);
    }
  }

  for (int i = 0; i < 10; ++i) {
    const bool r = SET_REMOVE(&foos, (struct foo){});
    if (i == 0) {
      ASSERT(r);
    } else {
      ASSERT(!r);
    }
    ASSERT_EQ(SET_SIZE(&foos), 0u);

    for (int j = 0; j < 10; ++j) {
      const bool present = SET_CONTAINS(&foos, (struct foo){});
      ASSERT(!present);
    }
  }

  SET_FREE(&foos);
}
