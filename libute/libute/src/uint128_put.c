/// @file
/// @brief Printing of 128-bit unsigned integers
///
/// All content in this file is in the public domain. Use it any way you wish.

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <ute/int128.h>

int uint128_put(uint128_t v, FILE *stream) {
  assert(stream != NULL);

  // working backwards, stringise the value into a temporary buffer
  char buffer[sizeof("340282366920938463463374607431768211455")] = {0};
  char *start = buffer + sizeof(buffer) - 1;
  do {
    assert(start > buffer);
    --start;
    *start = '0' + (char)(v % 10);
    v /= 10;
  } while (v != 0);

  // flush it to the stream
  return fputs(start, stream);
}
