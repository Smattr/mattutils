/// @file
/// @brief Atomically compare-and-swap a 128-bit unsigned integer
///
/// All content in this file is in the public domain. Use it any way you wish.

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <ute/int128.h>

bool uint128_atomic_cas_n(uint128_t *dst, uint128_t expected,
                          uint128_t desired) {
  assert(dst != NULL);

  return uint128_atomic_cas(dst, &expected, desired);
}
