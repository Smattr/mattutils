/// @file
/// @brief Test single-threaded atomic reading of a 128-bit signed integer
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <stdint.h>
#include <ute/int128.h>

TEST("int128_atomic_load single-threaded") {
  {
    int128_t x = 0;
    int128_t x2 = int128_atomic_load(&x);
    ASSERT(x == 0);
    ASSERT(x2 == 0);
  }

  {
    int128_t x = -1;
    int128_t x2 = int128_atomic_load(&x);
    ASSERT(x == -1);
    ASSERT(x2 == -1);
  }

  {
    int128_t x = 42;
    int128_t x2 = int128_atomic_load(&x);
    ASSERT(x == 42);
    ASSERT(x2 == 42);
  }

  {
    int128_t x = -42;
    int128_t x2 = int128_atomic_load(&x);
    ASSERT(x == -42);
    ASSERT(x2 == -42);
  }

  {
    int128_t x = (int128_t)UINT64_MAX + 1;
    int128_t x2 = int128_atomic_load(&x);
    ASSERT(x == (int128_t)UINT64_MAX + 1);
    ASSERT(x2 == (int128_t)UINT64_MAX + 1);
  }
}
