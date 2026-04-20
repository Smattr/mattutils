/// @file
/// @brief Implementation of lower half dword load
///
/// All content in this file is in the public domain. Use it any way you wish.

#include <assert.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <ute/dword.h>

#ifdef _MSC_VER
#define MAY_ALIAS /* nothing */
#else
#define MAY_ALIAS __attribute__((may_alias))
#endif

uintptr_t dword_atomic_load_lo(atomic_dword_t *src) {
  assert(src != NULL);

  typedef uintptr_t MAY_ALIAS word_t;
  _Atomic const word_t *const s = (_Atomic word_t *)src;
  return atomic_load_explicit(s, memory_order_acquire);
}
