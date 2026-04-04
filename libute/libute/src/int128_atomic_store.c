/// @file
/// @brief Atomically write a 128-bit signed integer
///
/// All content in this file is in the public domain. Use it any way you wish.

#include <assert.h>
#include <stddef.h>
#include <ute/int128.h>

#ifndef __has_include
#define __has_include(x) 0
#endif

#ifndef __has_feature
#define __has_feature(x) 0
#endif

#if __has_include(<immintrin.h>)
#include <immintrin.h>
#endif

void int128_atomic_store(int128_t *dst, int128_t src) {
  assert(dst != NULL);

#if __has_include(<immintrin.h>)
#if !__has_feature(thread_sanitizer)
#ifdef __SSE2__
  // A 128-bit AVX store is atomic. However _mm_store_si128 does not reliably
  // lower to a MOVDQA. Thankfully a volatile store seems to reliably lower to
  // either this or MOVAPS.
  {
    typedef __m128i __attribute__((may_alias)) avx128_t;

    volatile avx128_t *d = (avx128_t *)dst;
    *d = (__m128i)src;
  }
#endif
#endif
#endif

  // On 128-bit scalars, the __atomic built-ins and C11 atomics are not lowered
  // to native instructions. So we resort to an xchg.
  (void)int128_atomic_xchg(dst, src);
}
