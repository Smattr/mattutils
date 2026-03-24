/// @file
/// @brief Test printing of `INT128_MAX`
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <stdio.h>
#include <stdlib.h>
#include <ute/attr.h>
#include <ute/int128.h>

static char *buffer;
static size_t buffer_size;
static void free_(void *arg UNUSED) { free(buffer); }
static void fclose_(void *f) { (void)fclose(f); }

TEST("printing INT128_MAX") {

  // open a temporary stream
  register_cleanup(free_, NULL);
  FILE *const f = open_memstream(&buffer, &buffer_size);
  ASSERT_NOT_NULL(f);
  register_cleanup(fclose_, f);

  // print INT128_MAX to this
  {
    const int128_t max = INT128_MAX;
    const int r = int128_put(max, f);
    ASSERT_GE(r, 0);
    (void)fflush(f);
  }

  ASSERT_STREQ(buffer, "170141183460469231731687303715884105727");
}
