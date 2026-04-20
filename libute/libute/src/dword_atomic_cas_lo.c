/// @file
/// @brief Implementation of lower half dword CAS
///
/// All content in this file is in the public domain. Use it any way you wish.

#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <ute/dword.h>

#ifdef _MSC_VER
#define MAY_ALIAS /* nothing */
#else
#define MAY_ALIAS __attribute__((may_alias))
#endif

bool dword_atomic_cas_lo(atomic_dword_t *dst, uintptr_t *expected,
                         uintptr_t desired) {
  assert(dst != NULL);
  assert(expected != NULL);

  typedef uintptr_t MAY_ALIAS word_t;
  _Atomic word_t *const d = (_Atomic word_t *)dst;
  return atomic_compare_exchange_strong_explicit(
      d, expected, desired, memory_order_acq_rel, memory_order_acquire);
}
