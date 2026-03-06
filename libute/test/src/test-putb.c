/// @file
/// @brief Test `putb` works correctly
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ute/attr.h>
#include <ute/io.h>

static char *buffer;
static void free_(void *arg UNUSED) { free(buffer); }
static void fclose_(void *f) { (void)fclose(f); }

TEST("putb") {

  // open a temporary stream
  register_cleanup(free_, NULL);
  size_t buffer_size = 0;
  FILE *const f = open_memstream(&buffer, &buffer_size);
  ASSERT_NOT_NULL(f);
  register_cleanup(fclose_, f);

  // try stringizing various things
  struct {
    int8_t *data;
    char in_binary[17];
  } CASES[] = {
      {.data = (int8_t[]){0, 0}, .in_binary = "0000000000000000"},
      {.data = (int8_t[]){1, 0}, .in_binary = "1000000000000000"},
      {.data = (int8_t[]){0xff, 0}, .in_binary = "1111111100000000"},
      {.data = (int8_t[]){0x0f, 0xf0}, .in_binary = "1111000000001111"},
  };
  for (size_t i = 0; i < sizeof(CASES) / sizeof(CASES[0]); ++i) {
    const int r = putb(CASES[i].data, 2, f);
    ASSERT_EQ(r, 16);
    (void)fflush(f);
    ASSERT_STREQ(buffer, CASES[i].in_binary);

    // reset the stream
    memset(buffer, 0, buffer_size);
    rewind(f);
  }
}
