#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <ute/dword.h>

bool dword_atomic_cas_n(atomic_dword_t *dst, dword_t expected,
                        dword_t desired) {
  assert(dst != NULL);

  return dword_atomic_cas(dst, &expected, desired);
}
