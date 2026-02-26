/// @file
/// @brief Atomically write a 128-bit signed integer
///
/// All content in this file is in the public domain. Use it any way you wish.

#include <assert.h>
#include <stddef.h>
#include <ute/int128.h>

void int128_atomic_store(int128_t *dst, int128_t src) {
  assert(dst != NULL);

  // On 128-bit scalars, the __atomic built-ins and C11 atomics are not lowered
  // to native instructions. So we resort to an xchg.
  (void)int128_atomic_xchg(dst, src);
}
