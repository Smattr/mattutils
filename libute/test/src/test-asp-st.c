/// @file
/// @brief Basic single-threaded tests of atomic shared pointers
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <stdlib.h>
#include <ute/asp.h>
#include <ute/attr.h>

static void free_(void *p, void *context UNUSED) { free(p); }

/// count 0..10 by incrementing the value pointed to by a shared pointer
///
/// The failure mode of this test case will typically be:
///   1. Infinite loop if we have the logic for resolving the raw pointer from
///      the shared pointer wrong; or
///   2. ASan errors if we have the reference counting semantics wrong.
TEST("atomic shared pointer counter in-place, single-threaded") {

  asp_t ptr = 0;

  // a zero-initialized pointer should be null
  {
    const sp_t sp = sp_acq(&ptr);
    ASSERT_EQ(sp.ptr, NULL);
    sp_rel(sp);
  }

  // allocate an int pointer in this space
  {
    int *const p = malloc(sizeof(*p));
    ASSERT_NOT_NULL(p);
    *p = 0;
    const sp_t sp = sp_new(p, free_, NULL);
    ASSERT_NOT_NULL(sp.ptr);
    sp_store(&ptr, sp);
  }

  // count up to 10
  for (int i = 0; i < 10; ++i) {
    sp_t sp = sp_acq(&ptr);
    ASSERT_NOT_NULL(sp.ptr);
    int *const p = sp.ptr;
    ASSERT_EQ(*p, i);
    ++*p;
    sp_rel(sp);
  }

  // free the shared pointer by overwriting the last reference to it with null
  sp_t null = {0};
  sp_store(&ptr, null);
}

/// count 0..10 by allocating a new value each increment
///
/// The failure mode of this test case will typically be:
///   1. Infinite loop if we have the logic for resolving the raw pointer from
///      the shared pointer wrong; or
///   2. ASan errors if we have the reference counting semantics wrong.
TEST("atomic shared pointer counter reallocate, single-threaded") {

  asp_t ptr = 0;

  // a zero-initialized pointer should be null
  {
    const sp_t sp = sp_acq(&ptr);
    ASSERT_EQ(sp.ptr, NULL);
    sp_rel(sp);
  }

  // allocate an int pointer in this space
  {
    int *const p = malloc(sizeof(*p));
    ASSERT_NOT_NULL(p);
    *p = 0;
    const sp_t sp = sp_new(p, free_, NULL);
    ASSERT_NOT_NULL(sp.ptr);
    sp_store(&ptr, sp);
  }

  // count up to 10
  for (int i = 0; i < 10; ++i) {
    {
      sp_t sp = sp_acq(&ptr);
      ASSERT_NOT_NULL(sp.ptr);
      int *const p = sp.ptr;
      ASSERT_EQ(*p, i);
      sp_rel(sp);
    }
    // allocate a pointer with the new value
    int *const p = malloc(sizeof(*p));
    ASSERT_NOT_NULL(p);
    *p = i + 1;
    // overwrite the shared pointer with this
    const sp_t sp = sp_new(p, free_, NULL);
    ASSERT_NOT_NULL(sp.ptr);
    sp_store(&ptr, sp);
  }

  // free the shared pointer by overwriting the last reference to it with null
  sp_t null = {0};
  sp_store(&ptr, null);
}
