/// @file
/// @brief Implementation of dword store
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

void dword_atomic_store(atomic_dword_t *dst, dword_t src) {
  assert(dst != NULL);

#if defined(_MSC_VER) && defined(_WIN64)
  for (dword_t expected = dword_zero();;) {
    if (_InterlockedCompareExchange128(dst->word, src.word[1], src.word[0],
                                       expected.word)) {
      break;
    }
  }
#elif defined(_MSC_VER) && defined(_M_ARM)
  (void)_InterlockedExchange64_rel(dst, src);
#elif defined(_MSC_VER) && defined(_WIN32)
  for (dword_t old = dword_zero();;) {
    const dword_t expected = old;
    old = _InterlockedCompareExchange64(dst, src, expected);
    if (old == expected) {
      break;
    }
  }
#elif __SIZEOF_POINTER__ == 8
  uint128_atomic_store(dst, src);
#else
  atomic_store_explicit(dst, src, memory_order_release);
#endif
}
