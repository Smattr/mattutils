/// @file
/// @brief Test single-threaded atomic CAS of a 128-bit signed integer
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <stdint.h>
#include <ute/int128.h>

TEST("int128_atomic_cas single-threaded") {
  {
    int128_t x = 0;
    int128_t expected = x;
    const bool r = int128_atomic_cas(&x, &expected, 0);
    ASSERT(r);
    ASSERT(x == 0);
    ASSERT(expected == 0);
  }

  {
    int128_t x = 0;
    int128_t expected = x;
    const bool r = int128_atomic_cas(&x, &expected, 1);
    ASSERT(r);
    ASSERT(x == 1);
    ASSERT(expected == 0);
  }

  {
    int128_t x = 0;
    int128_t expected = x;
    const bool r = int128_atomic_cas(&x, &expected, -1);
    ASSERT(r);
    ASSERT(x == -1);
    ASSERT(expected == 0);
  }

  {
    int128_t x = 0;
    int128_t expected = x;
    const bool r = int128_atomic_cas(&x, &expected, 42);
    ASSERT(r);
    ASSERT(x == 42);
    ASSERT(expected == 0);
  }

  {
    int128_t x = 0;
    int128_t expected = x;
    const bool r = int128_atomic_cas(&x, &expected, -42);
    ASSERT(r);
    ASSERT(x == -42);
    ASSERT(expected == 0);
  }

  {
    int128_t x = 0;
    int128_t expected = x;
    const bool r = int128_atomic_cas(&x, &expected, (int128_t)UINT64_MAX + 1);
    ASSERT(r);
    ASSERT(x == (int128_t)UINT64_MAX + 1);
    ASSERT(expected == 0);
  }
}
