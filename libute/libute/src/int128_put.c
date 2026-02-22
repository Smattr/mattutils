/// @file
/// @brief Printing of 128-bit signed integers
///
/// All content in this file is in the public domain. Use it any way you wish.

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <ute/int128.h>

int int128_put(int128_t v, FILE *stream) {
  assert(stream != NULL);

  if (v >= 0)
    return uint128_put((uint128_t)v, stream);

  // avoid UB from negating `INT128_MIN`
  if (v == INT128_MIN)
    return fputs("-170141183460469231731687303715884105728", stream);

  {
    const int rc = putc('-', stream);
    if (rc < 0)
      return rc;
  }

  return uint128_put((uint128_t)-v, stream);
}
