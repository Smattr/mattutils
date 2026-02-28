#include <assert.h>
#include <stdatomic.h>
#include <stddef.h>
#include <ute/dword.h>

#if __SIZEOF_POINTER__ == 8
#include <ute/int128.h>
#endif

void dword_atomic_store(atomic_dword_t *dst, dword_t src) {
  assert(dst != NULL);

#if __SIZEOF_POINTER__ == 8
  uint128_atomic_store(dst, src);
#else
  atomic_store_explicit(dst, src, memory_order_release);
#endif
}
