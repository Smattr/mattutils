/// @file
/// @brief Implementation of dword exchange
///
/// All content in this file is in the public domain. Use it any way you wish.

#include <assert.h>
#include <stdatomic.h>
#include <stddef.h>
#include <ute/dword.h>

#ifdef _MSC_VER
#include <intrin.h>
#elif __SIZEOF_POINTER__ == 8
#include <ute/int128.h>
#endif

dword_t dword_atomic_xchg(atomic_dword_t *dst, dword_t src) {
  assert(dst != NULL);

#if defined(_MSC_VER) && defined(_M_ARM)
  return _InterlockedExchange64(dst, src);
#elif defined(_MSC_VER)
  {
    dword_t old = dword_zero();
    while (!dword_atomic_cas(dst, &old, src)) {
    }
    return old;
  }
#elif __SIZEOF_POINTER__ == 8
  return uint128_atomic_xchg(dst, src);
#else
  return atomic_exchange_explicit(dst, src, memory_order_acq_rel);
#endif
}
