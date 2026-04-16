/// @file
/// @brief Implementation of dword exchange
///
/// All content in this file is in the public domain. Use it any way you wish.

#include <assert.h>
#include <stdatomic.h>
#include <stddef.h>
#include <ute/dword.h>

#if __SIZEOF_POINTER__ == 8
#include <ute/int128.h>
#endif

dword_t dword_atomic_xchg(atomic_dword_t *dst, dword_t src) {
  assert(dst != NULL);

#if __SIZEOF_POINTER__ == 8
  return uint128_atomic_xchg(dst, src);
#else
  return atomic_exchange_explicit(dst, src, memory_order_acq_rel);
#endif
}
