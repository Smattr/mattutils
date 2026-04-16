/// @file
/// @brief Implementation of upper half dword CAS
///
/// All content in this file is in the public domain. Use it any way you wish.

#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <ute/dword.h>

bool dword_atomic_cas_hi(atomic_dword_t *dst, uintptr_t *expected,
                         uintptr_t desired) {
  assert(dst != NULL);
  assert(expected != NULL);

  typedef uintptr_t __attribute__((may_alias)) word_t;
  _Atomic word_t *const d = (_Atomic word_t *)dst;
  return atomic_compare_exchange_strong_explicit(
      d + 1, expected, desired, memory_order_acq_rel, memory_order_acquire);
}
