/// @file
/// @brief Implementation of dword load
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

dword_t dword_atomic_load(atomic_dword_t *src) {
  assert(src != NULL);

#if defined(_MSC_VER) && defined(_M_ARM64)
  {
    dword_t result = dword_zero();
    (void)_InterlockedCompareExchange128_acq(src->word, 0, 0, result.word);
    return result;
  }
#elif defined(_MSC_VER) && defined(_WIN64)
  {
    dword_t result = dword_zero();
    (void)_InterlockedCompareExchange128(src->word, 0, 0, result.word);
    return result;
  }
#elif defined(_MSC_VER) && defined(_M_ARM)
  return _InterlockedCompareExchange64_acq(src, 0, 0);
#elif defined(_MSC_VER) && defined(_WIN32)
  return _InterlockedCompareExchange64(src, 0, 0);
#elif __SIZEOF_POINTER__ == 8
  return uint128_atomic_load(src);
#else
  return atomic_load_explicit(src, memory_order_acquire);
#endif
}
