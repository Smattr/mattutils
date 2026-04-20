/// @file
/// @brief Implementation of dword CAS
///
/// All content in this file is in the public domain. Use it any way you wish.

#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <ute/dword.h>

#ifdef _MSC_VER
#include <intrin.h>
#elif __SIZEOF_POINTER__ == 8
#include <ute/int128.h>
#endif

bool dword_atomic_cas(atomic_dword_t *dst, dword_t *expected, dword_t desired) {
  assert(dst != NULL);
  assert(expected != NULL);

#if defined(_MSC_VER) && defined(_WIN64)
  {
    dword_t expectation = *expected;
    const unsigned char r = _InterlockedCompareExchange128(
        dst->word, desired.word[1], desired.word[0], expectation.word);
    if (r == 0)
      *expected = expectation;
    return r != 0;
  }
#elif defined(_MSC_VER) && defined(_WIN32)
  {
    const dword_t expectation = *expected;
    const dword_t old =
        _InterlockedCompareExchange64(dst, desired, expectation);
    if (old != expectation) {
      *expected = old;
      return false;
    }
    return true;
  }
#elif __SIZEOF_POINTER__ == 8
  return uint128_atomic_cas(dst, expected, desired);
#else
  return atomic_compare_exchange_strong_explicit(
      dst, expected, desired, memory_order_acq_rel, memory_order_acquire);
#endif
}
