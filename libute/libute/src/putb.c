/// @file
/// @brief Print a value in binary format
///
/// All content in this file is in the public domain. Use it any way you wish.

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <ute/print.h>

int putb(const void *data, size_t size, FILE *stream) {
  assert(data != NULL || size == 0);
  assert(stream != NULL);

  int written = 0;
  const char *d = data;
  for (size_t byte = 0; byte < size; ++byte) {
    for (size_t bit = 0; bit < CHAR_BIT; ++bit) {
      const char c = ((d[byte] >> bit) & 1) ? '1' : '0';
      if (putc(c, stream) < 0)
        return EOF;
      ++written;
    }
  }

  return written;
}
