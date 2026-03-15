/// @file
/// @brief Atomically read a 128-bit signed integer
///
/// All content in this file is in the public domain. Use it any way you wish.

#include <assert.h>
#include <stddef.h>
#include <ute/int128.h>

#ifdef __has_include
#if __has_include(<immintrin.h>)
#include <immintrin.h>

#ifdef __SSE2__
#define USE_AVX
#endif
#endif
#endif

#if !defined(USE_AVX) && !defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_16)
#error "__sync built-ins unavailable for 128-bit values"
#endif

int128_t int128_atomic_load(int128_t *src) {
  assert(src != NULL);

#ifdef USE_AVX
  // A 128-bit AVX load is atomic. However _mm_load_si128 does not reliably
  // lower to a MOVDQA. Thankfully a volatile load seems to reliably do this.
  {
    typedef __m128i __attribute__((may_alias)) avx128_t;

    volatile const avx128_t *s = (const avx128_t *)src;
    return (int128_t)*s;
  }
#else
  // On 128-bit scalars, the __atomic built-ins and C11 atomics are not lowered
  // to native instructions. So we resort to a __sync built-in. See:
  //   • https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80878
  //   • https://gcc.gnu.org/pipermail/gcc-help/2017-June.txt
  //   • https://gcc.gnu.org/bugzilla/show_bug.cgi?id=114310
  return __sync_val_compare_and_swap(src, 1, 1);
#endif
}
