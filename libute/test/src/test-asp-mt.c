/// @file
/// @brief Basic multi-threaded tests of atomic shared pointers
///
/// The tests in this file are essentially the same as the tests in
/// test-asp-st.c except these ones have each of 100 threads make one increment
/// each. This should result in the same functional effect, with no memory
/// leaks.
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ute/asp.h>
#include <ute/attr.h>

typedef struct {
  size_t thread_id;
  asp_t *ptr;
} state_t;

static THREAD_RET inplace_entry(void *arg) {
  assert(arg != NULL);
  state_t *const s = arg;

  while (true) {
    // load the value of the pointer
    const sp_t sp = sp_acq(s->ptr);

    // unless this is our thread ID, retry
    _Atomic size_t *const id_ptr = sp.ptr;
    const size_t id = atomic_load_explicit(id_ptr, memory_order_acquire);
    if (id != s->thread_id) {
      sp_rel(sp);
      continue;
    }

    // increment to the next thread’s ID
    (void)atomic_fetch_add_explicit(id_ptr, 1, memory_order_acq_rel);

    sp_rel(sp);
    break;
  }

  return 0;
}

static void free_(void *p, void *context UNUSED) { free(p); }

/// count 0..10 by incrementing the value pointed to by a shared pointer
///
/// The failure mode of this test case will typically be:
///   1. Infinite loop if we have the logic for resolving the raw pointer from
///      the shared pointer wrong; or
///   2. ASan errors if we have the reference counting semantics wrong.
TEST("atomic shared pointer counter in-place, multi-threaded") {

  asp_t ptr = 0;

  // a zero-initialized pointer should be null
  {
    const sp_t sp = sp_acq(&ptr);
    ASSERT_EQ(sp.ptr, NULL);
    sp_rel(sp);
  }

  // allocate a size_t pointer in this space
  {
    _Atomic size_t *const p = malloc(sizeof(*p));
    ASSERT_NOT_NULL(p);
    atomic_store_explicit(p, 0, memory_order_release);
    const sp_t sp = sp_new(p, free_, NULL);
    ASSERT_NOT_NULL(sp.ptr);
    sp_store(&ptr, sp);
  }

  thread_t t[100];
  state_t s[sizeof(t) / sizeof(t[0])];

  // setup states first to avoid racing our reads of `target` with the test
  // case’s writes
  for (size_t i = 0; i < sizeof(s) / sizeof(s[0]); ++i)
    s[i] = (state_t){.thread_id = i, .ptr = &ptr};

  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    const int r = THREAD_CREATE(&t[i], inplace_entry, &s[i]);
    ASSERT_EQ(r, 0);
  }

  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    THREAD_RET ret = 0;
    const int r = THREAD_JOIN(t[i], &ret);
    ASSERT_EQ(r, 0);
    ASSERT_EQ(ret, (THREAD_RET){0});
  }

  // free the shared pointer by overwriting the last reference to it with null
  const sp_t null = {0};
  sp_store(&ptr, null);
}

static THREAD_RET reallocate_entry(void *arg) {
  assert(arg != NULL);
  state_t *const s = arg;

  while (true) {
    // load the value of the pointer
    const sp_t sp = sp_acq(s->ptr);

    // if this was null and we are the first thread…
    if (sp.ptr == NULL && s->thread_id == 0) {
      // …write 1
      _Atomic size_t *const p = malloc(sizeof(*p));
      ASSERT_NOT_NULL(p);
      atomic_store_explicit(p, 1, memory_order_release);
      const sp_t new = sp_new(p, free_, NULL);
      ASSERT_NOT_NULL(new.ptr);
      sp_store(s->ptr, new);
      break;
    }

    // if this is not our thread ID, retry
    _Atomic size_t *const id_ptr = sp.ptr;
    const size_t id = atomic_load_explicit(id_ptr, memory_order_acquire);
    if (id != s->thread_id) {
      sp_rel(sp);
      continue;
    }

    // create a new pointer with the next thread’s ID
    _Atomic size_t *const next = malloc(sizeof(*next));
    ASSERT_NOT_NULL(next);
    atomic_store_explicit(next, s->thread_id + 1, memory_order_release);
    const sp_t new = sp_new(next, free_, NULL);
    ASSERT_NOT_NULL(new.ptr);
    sp_store(s->ptr, new);

    sp_rel(sp);
    break;
  }

  return 0;
}

/// count 0..10 by allocating a new value each increment
///
/// The failure mode of this test case will typically be:
///   1. Infinite loop if we have the logic for resolving the raw pointer from
///      the shared pointer wrong; or
///   2. ASan errors if we have the reference counting semantics wrong.
TEST("atomic shared pointer counter reallocate, multi-threaded") {

  asp_t ptr = 0;

  // a zero-initialized pointer should be null
  {
    const sp_t sp = sp_acq(&ptr);
    ASSERT_EQ(sp.ptr, NULL);
    sp_rel(sp);
  }

  // allocate a size_t pointer in this space
  {
    _Atomic size_t *const p = malloc(sizeof(*p));
    ASSERT_NOT_NULL(p);
    atomic_store_explicit(p, 0, memory_order_release);
    const sp_t sp = sp_new(p, free_, NULL);
    ASSERT_NOT_NULL(sp.ptr);
    sp_store(&ptr, sp);
  }

  thread_t t[100];
  state_t s[sizeof(t) / sizeof(t[0])];

  // setup states first to avoid racing our reads of `target` with the test
  // case’s writes
  for (size_t i = 0; i < sizeof(s) / sizeof(s[0]); ++i)
    s[i] = (state_t){.thread_id = i, .ptr = &ptr};

  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    const int r = THREAD_CREATE(&t[i], reallocate_entry, &s[i]);
    ASSERT_EQ(r, 0);
  }

  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    THREAD_RET ret = 0;
    const int r = THREAD_JOIN(t[i], &ret);
    ASSERT_EQ(r, 0);
    ASSERT_EQ(ret, (THREAD_RET){0});
  }

  // free the shared pointer by overwriting the last reference to it with null
  const sp_t null = {0};
  sp_store(&ptr, null);
}

static THREAD_RET write_entry(void *arg) {
  assert(arg != NULL);
  state_t *const s = arg;

  // ignore the current value of the pointer, and write our own
  _Atomic size_t *const p = malloc(sizeof(*p));
  ASSERT_NOT_NULL(p);
  atomic_store_explicit(p, s->thread_id, memory_order_release);
  const sp_t new = sp_new(p, free_, NULL);
  ASSERT_NOT_NULL(new.ptr);
  sp_store(s->ptr, new);

  return 0;
}

/// rather than trying to cooperate, have all threads race each other to write
///
/// The failure mode of this test case will typically be ASan observing memory
/// leaks if we have the reference counting semantics wrong.
TEST("atomic shared pointer counter overwrite, multi-threaded") {

  asp_t ptr = 0;

  // a zero-initialized pointer should be null
  {
    const sp_t sp = sp_acq(&ptr);
    ASSERT_EQ(sp.ptr, NULL);
    sp_rel(sp);
  }

  thread_t t[100];
  state_t s[sizeof(t) / sizeof(t[0])];

  // setup states first to avoid racing our reads of `target` with the test
  // case’s writes
  for (size_t i = 0; i < sizeof(s) / sizeof(s[0]); ++i)
    s[i] = (state_t){.thread_id = i, .ptr = &ptr};

  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    const int r = THREAD_CREATE(&t[i], write_entry, &s[i]);
    ASSERT_EQ(r, 0);
  }

  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    THREAD_RET ret = 0;
    const int r = THREAD_JOIN(t[i], &ret);
    ASSERT_EQ(r, 0);
    ASSERT_EQ(ret, (THREAD_RET){0});
  }

  // free the shared pointer by overwriting the last reference to it with null
  const sp_t null = {0};
  sp_store(&ptr, null);
}
