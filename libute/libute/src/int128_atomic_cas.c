/// @file
/// @brief Atomically compare-and-swap a 128-bit signed integer
///
/// All content in this file is in the public domain. Use it any way you wish.

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <ute/int128.h>

#ifndef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_16
#error "__sync built-ins unavailable for 128-bit values"
#endif

bool int128_atomic_cas(int128_t *dst, int128_t *expected, int128_t desired) {
  assert(dst != NULL);
  assert(expected != NULL);

  // On 128-bit scalars, the __atomic built-ins and C11 atomics are not lowered
  // to native instructions. So we resort to a __sync built-in. See:
  //   • https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80878
  //   • https://gcc.gnu.org/pipermail/gcc-help/2017-June.txt
  const int128_t expectation = *expected;
  const int128_t old = __sync_val_compare_and_swap(dst, expectation, desired);
  if (old == expectation)
    return true;
  *expected = old;
  return false;
}
