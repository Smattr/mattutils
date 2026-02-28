/// @file
/// @brief Atomically write a 128-bit unsigned integer
///
/// All content in this file is in the public domain. Use it any way you wish.

#include <assert.h>
#include <stddef.h>
#include <ute/int128.h>

void uint128_atomic_store(uint128_t *dst, uint128_t src) {
  assert(dst != NULL);

  // On 128-bit scalars, the __atomic built-ins and C11 atomics are not lowered
  // to native instructions. So we resort to an xchg.
  (void)uint128_atomic_xchg(dst, src);
}
