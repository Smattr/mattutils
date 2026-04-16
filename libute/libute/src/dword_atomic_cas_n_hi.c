/// @file
/// @brief Implementation of upper half dword CAS by value
///
/// All content in this file is in the public domain. Use it any way you wish.

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <ute/dword.h>

bool dword_atomic_cas_n_hi(atomic_dword_t *dst, uintptr_t expected,
                           uintptr_t desired) {
  assert(dst != NULL);

  return dword_atomic_cas_hi(dst, &expected, desired);
}
