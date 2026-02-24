/// @file
/// @brief Test printing of small values of `uint128_t`
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ute/attr.h>
#include <ute/int128.h>

static char *buffer;
static void free_(void *arg UNUSED) { free(buffer); }
static void fclose_(void *f) { (void)fclose(f); }

TEST("printing small uint128_ts") {

  // open a temporary stream
  register_cleanup(free_, NULL);
  size_t buffer_size = 0;
  FILE *const f = open_memstream(&buffer, &buffer_size);
  ASSERT_NOT_NULL(f);
  register_cleanup(fclose_, f);

  // print some small values
  for (unsigned i = 0; i <= 100; ++i) {

    // print it
    const uint128_t v = i;
    const int r = uint128_put(v, f);
    ASSERT_GE(r, 0);
    (void)fflush(f);

    // was it correctly printed?
    char reference[sizeof("100")] = {0};
    snprintf(reference, sizeof(reference), "%u", i);
    ASSERT_STREQ(buffer, reference);

    // reset the buffer
    memset(buffer, 0, buffer_size);
    rewind(f);
  }
}
