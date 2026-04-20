/// @file
/// @brief Implementation of lower half dword store
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

void dword_atomic_store_lo(atomic_dword_t *dst, uintptr_t src) {
  assert(dst != NULL);

  typedef uintptr_t MAY_ALIAS word_t;
  _Atomic word_t *const d = (_Atomic word_t *)dst;
  atomic_store_explicit(d, src, memory_order_release);
}
