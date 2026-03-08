/// @file
/// @brief Race `sp_acq` and `sp_store` with one another
///
/// In constructing a formal model of asp.c, it was discovered that `sp_acq`
/// suffered from an ABA race:
///   1. Thread A in `sp_acq` increments load count
///   2. Thread A in `sp_acq` increments ref count
///   3. Thread B runs `sp_store` to completion but with the same pointer as is
///      already stored. Note that this sets the load count to 0.
///   4. Thread A in `sp_acq` tries to decrement load count, but fails because
///      it is 0, not 1 as the thread expected
///   5. Thread A sees `impl.ctrl == updated.ctrl`
///   6. Thread A decrements load count, underflowing
/// (6) did not actually happen in debug builds because this is caught by an
/// assertion failure.
///
/// The problem occurred because thread A believed it was seeing an unchanged
/// pointer, but it was actually seeing something that had changed but to the
/// same value it was expecting. Simply restricting the store sources to
/// differing pointers to that which is already stored (would require making
/// `sp_store` use a CAS instead) does not solve the problem. The above sequence
/// of steps is really an “AA” problem; it can be elongated into an ABA problem
/// by interleaving a store of a different pointer in the middle, which would
/// side step this naive fix.
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ute/asp.h>

typedef struct {
  size_t thread_id;
  asp_t *ptr;
} state_t;

/// entry point for threads that will perform repeated self-stores to the asp
static THREAD_RET store(void *arg) {
  assert(arg != NULL);
  state_t *const s = arg;

  for (size_t i = 0; i < 10; ++i) {
    // load the value of the pointer
    const sp_t sp = sp_acq(s->ptr);

    // store it back to the same location
    sp_store(s->ptr, sp);
  }
  return 0;
}

/// entry point for the threads that will count up
static THREAD_RET count(void *arg) {
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

    // increment to the next counter thread’s ID
    (void)atomic_fetch_add_explicit(id_ptr, 2, memory_order_acq_rel);

    sp_rel(sp);
    break;
  }

  return 0;
}

/// race a group of threads, half counting up and half self-storing
///
/// The failure mode of this test case will typically be:
///   1. An assertion failure due to incorrect manipulation of asp load counts;
///      or
///   2. ASan reporting leaks from miscounted shared pointers.
TEST("atomic shared pointer store/acquire race") {

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
    const sp_t sp = sp_new(p, free);
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
    const int r = THREAD_CREATE(&t[i], i % 2 == 0 ? count : store, &s[i]);
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
