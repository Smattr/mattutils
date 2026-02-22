/// @file
/// @brief Test printing of `UINT128_MAX`
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <ute/int128.h>

static char *buffer;
static void free_(void *arg __attribute__((unused))) { free(buffer); }
static void fclose_(void *f) { (void)fclose(f); }

TEST("printing UINT128_MAX") {

  // open a temporary stream
  register_cleanup(free_, NULL);
  size_t buffer_size = 0;
  FILE *const f = open_memstream(&buffer, &buffer_size);
  ASSERT_NOT_NULL(f);
  register_cleanup(fclose_, f);

  // print UINT128_MAX to this
  {
    const uint128_t max = UINT128_MAX;
    const int r = uint128_put(max, f);
    ASSERT_GE(r, 0);
    (void)fflush(f);
  }

  ASSERT_STREQ(buffer, "340282366920938463463374607431768211455");
}
