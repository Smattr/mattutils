/// @file
/// @brief Test single-threaded atomic reading of a 128-bit unsigned integer
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <stdint.h>
#include <ute/int128.h>

TEST("uint128_atomic_load single-threaded") {
  {
    uint128_t x = 0;
    uint128_t x2 = uint128_atomic_load(&x);
    ASSERT(x == 0);
    ASSERT(x2 == 0);
  }

  {
    uint128_t x = 42;
    uint128_t x2 = uint128_atomic_load(&x);
    ASSERT(x == 42);
    ASSERT(x2 == 42);
  }

  {
    uint128_t x = (uint128_t)UINT64_MAX + 1;
    uint128_t x2 = uint128_atomic_load(&x);
    ASSERT(x == (uint128_t)UINT64_MAX + 1);
    ASSERT(x2 == (uint128_t)UINT64_MAX + 1);
  }
}
