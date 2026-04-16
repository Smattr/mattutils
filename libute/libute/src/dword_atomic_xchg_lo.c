/// @file
/// @brief Implementation of lower half dword exchange
///
/// All content in this file is in the public domain. Use it any way you wish.

#include <assert.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <ute/dword.h>

uintptr_t dword_atomic_xchg_lo(atomic_dword_t *dst, uintptr_t src) {
  assert(dst != NULL);

  typedef uintptr_t __attribute__((may_alias)) word_t;
  _Atomic word_t *const d = (_Atomic word_t *)dst;
  return atomic_exchange_explicit(d, src, memory_order_acq_rel);
}
