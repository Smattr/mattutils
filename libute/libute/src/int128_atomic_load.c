/// @file 

#include <ute/int128.h>
#include <assert.h>
#include <stddef.h>

#ifndef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_16
#error "__sync built-ins unavailable for 128-bit values"
#endif

int128_t int128_atomic_load(int128_t *src) {
  assert(src != NULL);

  // On 128-bit scalars, the __atomic built-ins and C11 atomics are not lowered
  // to native instructions. So we resort to a __sync built-in. See:
  //   • https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80878
  //   • https://gcc.gnu.org/pipermail/gcc-help/2017-June.txt
  //   • https://gcc.gnu.org/bugzilla/show_bug.cgi?id=114310
  return __sync_val_compare_and_swap(src, 1, 1);
}
