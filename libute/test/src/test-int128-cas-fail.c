/// @file
/// @brief Test single-threaded failing atomic CAS of a 128-bit signed integer
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <stdint.h>
#include <ute/int128.h>

TEST("int128_atomic_cas fail single-threaded") {
  {
    int128_t x = 0;
    int128_t expected = x + 1; // an expectation that is incorrect
    const bool r = int128_atomic_cas(&x, &expected, 0);
    // CAS should have failed
    ASSERT(!r);
    // failing CAS should not have modified the target
    ASSERT(x == 0);
    // expectation should have been updated
    ASSERT(expected == 0);
  }

  {
    int128_t x = 1;
    int128_t expected = x + 1; // an expectation that is incorrect
    const bool r = int128_atomic_cas(&x, &expected, -1);
    // CAS should have failed
    ASSERT(!r);
    // failing CAS should not have modified the target
    ASSERT(x == 1);
    // expectation should have been updated
    ASSERT(expected == 1);
  }

  {
    int128_t x = -1;
    int128_t expected = x + 1; // an expectation that is incorrect
    const bool r = int128_atomic_cas(&x, &expected, 1);
    // CAS should have failed
    ASSERT(!r);
    // failing CAS should not have modified the target
    ASSERT(x == -1);
    // expectation should have been updated
    ASSERT(expected == -1);
  }

  {
    int128_t x = 42;
    int128_t expected = x + 1; // an expectation that is incorrect
    const bool r = int128_atomic_cas(&x, &expected, -42);
    // CAS should have failed
    ASSERT(!r);
    // failing CAS should not have modified the target
    ASSERT(x == 42);
    // expectation should have been updated
    ASSERT(expected == 42);
  }

  {
    int128_t x = -42;
    int128_t expected = x + 1; // an expectation that is incorrect
    const bool r = int128_atomic_cas(&x, &expected, 42);
    // CAS should have failed
    ASSERT(!r);
    // failing CAS should not have modified the target
    ASSERT(x == -42);
    // expectation should have been updated
    ASSERT(expected == -42);
  }

  {
    int128_t x = (int128_t)UINT64_MAX + 1;
    int128_t expected = x + 1; // an expectation that is incorrect
    const bool r = int128_atomic_cas(&x, &expected, INT64_MIN);
    // CAS should have failed
    ASSERT(!r);
    // failing CAS should not have modified the target
    ASSERT(x == (int128_t)UINT64_MAX + 1);
    // expectation should have been updated
    ASSERT(expected == (int128_t)UINT64_MAX + 1);
  }
}
