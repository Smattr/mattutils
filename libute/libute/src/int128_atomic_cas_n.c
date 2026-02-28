/// @file
/// @brief Atomically compare-and-swap a 128-bit signed integer
///
/// All content in this file is in the public domain. Use it any way you wish.

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <ute/int128.h>

bool int128_atomic_cas_n(int128_t *dst, int128_t expected, int128_t desired) {
  assert(dst != NULL);

  return int128_atomic_cas(dst, &expected, desired);
}
