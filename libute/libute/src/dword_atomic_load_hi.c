/// @file
/// @brief Implementation of upper half dword load
///
/// All content in this file is in the public domain. Use it any way you wish.

#include <assert.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <ute/dword.h>

uintptr_t dword_atomic_load_hi(atomic_dword_t *src) {
  assert(src != NULL);

  typedef uintptr_t __attribute__((may_alias)) word_t;
  _Atomic const word_t *const s = (_Atomic word_t *)src;
  return atomic_load_explicit(s + 1, memory_order_acquire);
}
