/// @file
/// @brief Atomically exchange two 128-bit unsigned integers
///
/// All content in this file is in the public domain. Use it any way you wish.

#include <assert.h>
#include <stddef.h>
#include <ute/int128.h>

#ifndef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_16
#error "__sync built-ins unavailable for 128-bit values"
#endif

uint128_t uint128_atomic_xchg(uint128_t *dst, uint128_t src) {
  assert(dst != NULL);

  // On 128-bit scalars on (at least) aarch64 and x86-64, the __atomic built-ins
  // and C11 atomics are not lowered to native instructions. So we resort to a
  // __sync-built-in-based CAS loop. See:
  //   • https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80878
  //   • https://gcc.gnu.org/pipermail/gcc-help/2017-June.txt
  for (uint128_t expected = 0;;) {
    const uint128_t old = __sync_val_compare_and_swap(dst, expected, src);
    if (old == expected)
      return old;
    expected = old;
  }
}
