/// @file
/// @brief Implementation of dword load
///
/// All content in this file is in the public domain. Use it any way you wish.

#include <assert.h>
#include <stdatomic.h>
#include <stddef.h>
#include <ute/dword.h>

#if __SIZEOF_POINTER__ == 8
#include <ute/int128.h>
#endif

dword_t dword_atomic_load(atomic_dword_t *src) {
  assert(src != NULL);

#if __SIZEOF_POINTER__ == 8
  return uint128_atomic_load(src);
#else
  return atomic_load_explicit(src, memory_order_acquire);
#endif
}
