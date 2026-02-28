/// @file
/// @brief Test single-threaded failing atomic CAS of a 128-bit unsigned integer
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <stdint.h>
#include <ute/int128.h>

TEST("uint128_atomic_cas fail single-threaded") {
  {
    uint128_t x = 0;
    uint128_t expected = x + 1; // an expectation that is incorrect
    const bool r = uint128_atomic_cas(&x, &expected, 0);
    // CAS should have failed
    ASSERT(!r);
    // failing CAS should not have modified the target
    ASSERT(x == 0);
    // expectation should have been updated
    ASSERT(expected == 0);
  }

  {
    uint128_t x = 1;
    uint128_t expected = x + 1; // an expectation that is incorrect
    const bool r = uint128_atomic_cas(&x, &expected, 2);
    // CAS should have failed
    ASSERT(!r);
    // failing CAS should not have modified the target
    ASSERT(x == 1);
    // expectation should have been updated
    ASSERT(expected == 1);
  }

  {
    uint128_t x = 42;
    uint128_t expected = x + 1; // an expectation that is incorrect
    const bool r = uint128_atomic_cas(&x, &expected, 43);
    // CAS should have failed
    ASSERT(!r);
    // failing CAS should not have modified the target
    ASSERT(x == 42);
    // expectation should have been updated
    ASSERT(expected == 42);
  }

  {
    uint128_t x = (uint128_t)UINT64_MAX + 1;
    uint128_t expected = x + 1; // an expectation that is incorrect
    const bool r = uint128_atomic_cas(&x, &expected, INT64_MAX);
    // CAS should have failed
    ASSERT(!r);
    // failing CAS should not have modified the target
    ASSERT(x == (uint128_t)UINT64_MAX + 1);
    // expectation should have been updated
    ASSERT(expected == (uint128_t)UINT64_MAX + 1);
  }
}
