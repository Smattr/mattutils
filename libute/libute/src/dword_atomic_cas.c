#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <ute/dword.h>

#if __SIZEOF_POINTER__ == 8
#include <ute/int128.h>
#endif

bool dword_atomic_cas(atomic_dword_t *dst, dword_t *expected, dword_t desired) {
  assert(dst != NULL);
  assert(expected != NULL);

#if __SIZEOF_POINTER__ == 8
  return uint128_atomic_cas(dst, expected, desired);
#else
  return atomic_compare_exchange_strong_explicit(
      dst, expected, desired, memory_order_acq_rel, memory_order_acquire);
#endif
}
