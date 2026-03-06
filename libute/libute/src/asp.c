/// @file
/// @brief Implementation of the asp.h API
///
/// Implementing an atomically-reference-counted pointer in the style of C++’s
/// `std::shared_ptr` is relatively straightforward. However the API we are
/// implementing is an _atomic_ atomically-reference-counter pointer. That is,
/// similar a C++’s `std::atomic<std::shared_ptr<…>>`, a pointer that cannot
/// only be thread-safely shared but also thread-safely replaced without leaks.
///
/// A lock-free atomic reference-counted pointer is quite an elusive creature.
///
/// The technique we use was, I believe, invented by Anthony Williams. A good
/// explanation of it is Timur Doumler’s ACCU 2022 talk:
///
///   A lock-free std::atomic<std::shared_ptr>
///   Timur Doumler, ACCU 2022
///   https://www.youtube.com/watch?v=a10JpqI-CvU
///
/// For a comparable alternative summary:
///
///   Lock-free atomic shared pointers without a split reference count? It can
///   be done!
///   Daniel Anderson, CppCon 2023
///   https://www.youtube.com/watch?v=lNPZV9Iqo3U
///
/// The above two talks cover a lot of alternatives in this space including
/// Hazard Pointers and Read-Copy-Update, but we have stuck to split reference
/// counts. This avoids the need for a runtime or to know the total thread
/// count.
///
/// On top of the basic implementation shown in the above talks, we implement an
/// optimization of not reference-counting the null pointer.¹ Perhaps this
/// reduces contention, but the real advantage of this is that a
/// zero-initialized `asp_t` is a valid pointer to null. Similarly a
/// zero-initialized `sp_t` is a valid reference to null.
///
/// A quirk that is _not_ shown in the above talks is that the reference count
/// stored in the control block is split into two halves, a reference count and
/// a load count. This is necessary to avoid a use-after-free (see commit
/// history of this file), but this requirement is non-obvious and seems to have
/// been arrived at independently by multiple authors including yours truly.
///
/// The asp.h API is phrased such that it is agnostic to how exactly the
/// “shared” part of “shared pointer” is achieved. We choose to use reference
/// counting. But in theory, if you are creative enough, you could implement
/// the API using a different technique.
///
/// ¹ In Raymond Chen’s terminology, we avoid “phantom” and “indulgent”
///   pointers:
///     https://devblogs.microsoft.com/oldnewthing/20241219-00/?p=110663
///     https://devblogs.microsoft.com/oldnewthing/20230818-00/?p=108619

#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ute/asp.h>
#include <ute/dword.h>

// uncomment one or more of these to debug unlikely thread interleavings
// #define DELAY1() sleep(1)
// #define DELAY2() sleep(1)
// #define DELAY3() sleep(1)

#ifndef DELAY1
#define DELAY1()                                                               \
  do {                                                                         \
  } while (0)
#endif
#ifndef DELAY2
#define DELAY2()                                                               \
  do {                                                                         \
  } while (0)
#endif
#ifndef DELAY3
#define DELAY3()                                                               \
  do {                                                                         \
  } while (0)
#endif

#ifndef __SIZEOF_POINTER__
#error "sizeof pointer not available"
#endif

/// signed and unsigned half words
#if __SIZEOF_POINTER__ == 8
typedef int32_t ihalf_t;
typedef uint32_t uhalf_t;
#define IHALF_MAX INT32_MAX
#define IHALF_MIN INT32_MIN
#define UHALF_MAX UINT32_MAX
#elif __SIZEOF_POINTER__ == 4
typedef int16_t ihalf_t;
typedef uint16_t uhalf_t;
#define IHALF_MAX INT16_MAX
#define IHALF_MIN INT16_MIN
#define UHALF_MAX UINT16_MAX
#error "unimplemented"
#endif

/// reference count of a control block
///
/// This is split into two counts that must be operated on atomically together.
/// It is only permitted to change `loads` when you hold a reference, thus
/// preventing `refs` to go to 0 during this. Nothing explicitly enforces this.
///
/// The reference count is only logically 0 (allowing clean up) when _both_
/// `refs` and `loads` are 0.
///
/// The overflow assertions later in this file should be impossible to trigger
/// in practice because they require ≥IHALF_MAX threads operating on the same
/// shared pointer.
typedef struct {
  /// number of outstanding live references to this control block
  ///
  /// This is effectively the number of live shared pointers to the pointer this
  /// count is associated with.
  uhalf_t refs;

  /// number of live loads of this control block that will be unable to roll
  /// back
  ///
  /// When loading an atomic shared pointer, the load count is first incremented
  /// and then later decremented. If this counter is relocated, making the
  /// roll back decrement not possible, the residual load counter value will be
  /// relocated to here. Note that this value is signed because the order in
  /// which the decrementer and relocater run is unconstrained; this can go
  /// negative.
  ihalf_t loads;
} rc_t;

static rc_t rc_load(_Atomic rc_t *src) {
  assert(src != NULL);
  return atomic_load_explicit(src, memory_order_acquire);
}

static bool rc_cas(_Atomic rc_t *dst, rc_t *expected, rc_t desired) {
  assert(dst != NULL);
  assert(expected != NULL);
  return atomic_compare_exchange_strong_explicit(
      dst, expected, desired, memory_order_acq_rel, memory_order_acquire);
}

struct sp_ctrl {
  void *value;          ///< the managed underlying pointer
  void (*dtor)(void *); ///< optional user-supplied destructor
  _Atomic rc_t rc;      ///< outstanding references
};

/// increment the reference count of a shared pointer
///
/// @param ctrl Control block to operate on
/// @param by Number of references to add
static void inc_ref(sp_ctrl_t *ctrl, size_t by) {
  assert(ctrl != NULL);
  assert(by <= UHALF_MAX && "overflow");

  for (rc_t old = {0};;) {
    assert(old.refs + by <= UHALF_MAX);
    const rc_t new = {.refs = old.refs + (uhalf_t)by, .loads = old.loads};
    if (rc_cas(&ctrl->rc, &old, new))
      break;
  }
}

/// propagate an increment of the load count of a shared pointer
///
/// @param ctrl Control block to operate on
/// @param by Number of loads to add
static void inc_load_ref(sp_ctrl_t *ctrl, size_t by) {
  assert(ctrl != NULL);
  assert(by <= (size_t)IHALF_MAX && "overflow");

  for (rc_t old = rc_load(&ctrl->rc);;) {
    assert(old.refs > 0 && "changing load count while not holding a reference");
    assert(IHALF_MAX - (ihalf_t)by >= old.loads && "overflow");
    const rc_t new = {.refs = old.refs, .loads = old.loads + (ihalf_t)by};
    if (rc_cas(&ctrl->rc, &old, new))
      break;
  }
}

/// decrement the reference count of a shared pointer by 1
///
/// @param ctrl Control block to operate on
static void dec_ref(sp_ctrl_t *ctrl) {
  assert(ctrl != NULL);

  rc_t old = rc_load(&ctrl->rc);
  while (true) {
    assert(old.refs > 0 && "dropping a reference that was not held");
    const rc_t new = {.refs = old.refs - 1, .loads = old.loads};
    if (rc_cas(&ctrl->rc, &old, new))
      break;
  }

  // if we just dropped the last reference, clean up
  if (old.refs == 1 && old.loads == 0) {
    if (ctrl->dtor != NULL)
      ctrl->dtor(ctrl->value);
    free(ctrl);
  }
}

/// decrement the propagated load count of a shared pointer by 1
///
/// @param ctrl Control block to operate on
static void dec_load_ref(sp_ctrl_t *ctrl) {
  assert(ctrl != NULL);

  for (rc_t old = rc_load(&ctrl->rc);;) {
    assert(old.refs > 0 && "changing load count while not holding a reference");
    assert(old.loads > IHALF_MIN && "overflow");
    const rc_t new = {.refs = old.refs, .loads = old.loads - 1};
    if (rc_cas(&ctrl->rc, &old, new))
      break;
  }
}

/// exposed implementation of an atomic shared pointer
///
/// External callers see atomic shared pointers as `atomic_dword_t`s. But we
/// operate on them as `sp_impl_t`s.
typedef struct {
  sp_ctrl_t *ctrl;   ///< control block, only relevant for non-null pointers
  size_t load_count; ///< number of outstanding control block loads
} asp_impl_t;

/// unwrap the external representation into our representation
static asp_impl_t asp2impl(dword_t src) {
  asp_impl_t dst;
  static_assert(sizeof(dst) <= sizeof(src),
                "atomic shared pointer does not fit in a dword");
  memcpy(&dst, &src, sizeof(dst));
  return dst;
}

/// wrap our representation into the external representation
static dword_t impl2asp(asp_impl_t src) {
  dword_t dst = 0;
  static_assert(sizeof(src) <= sizeof(dst),
                "atomic shared pointer does not fit in a dword");
  memcpy(&dst, &src, sizeof(src));
  return dst;
}

sp_t sp_new(void *value, void (*dtor)(void *)) {

  // a null pointer needs no bookkeeping
  if (value == NULL) {
    return (sp_t){0};
  }

  sp_ctrl_t *const ctrl = calloc(1, sizeof(*ctrl));
  if (ctrl == NULL) {
    return (sp_t){0};
  }

  ctrl->value = value;
  ctrl->dtor = dtor;
  inc_ref(ctrl, 1);

  return (sp_t){.ptr = value, .impl = ctrl};
}

sp_t sp_acq(asp_t *asp) {
  assert(asp != NULL);

  // Disclaimer: the justification for why the logic in this function is correct
  // is very subtle. It is extremely hard to understand this from the code
  // alone. Read the header comment in this file and study the referenced
  // presentations before trying to read this code.

  // load the implementation, incrementing the load count
  dword_t old = dword_atomic_load(asp);
  while (true) {
    asp_impl_t impl = asp2impl(old);
    if (impl.ctrl == NULL) { // the target pointer is null; no need to ref count
      return (sp_t){0};
    }
    ++impl.load_count;
    const dword_t new = impl2asp(impl);
    if (dword_atomic_cas(asp, &old, new)) {
      old = new;
      break;
    }
  }

  DELAY1();

  asp_impl_t impl = asp2impl(old);
  assert(impl.ctrl != NULL && "non-null pointer has no control block");

  // load the target pointer, incrementing the reference count
  inc_ref(impl.ctrl, 1);
  const sp_t ret = {.ptr = impl.ctrl->value, .impl = impl.ctrl};

  // undo our increment of the load count
  while (true) {
    assert(impl.load_count > 0 &&
           "sp_acq decremented a load for a control block it was not using");
    --impl.load_count;
    const dword_t new = impl2asp(impl);
    if (dword_atomic_cas(asp, &old, new))
      break;
    const asp_impl_t updated = asp2impl(old);
    if (impl.ctrl != updated.ctrl) {
      // XXX: this is the counter-intuitive path
      DELAY2();
      dec_load_ref(impl.ctrl);
      break;
    }
    impl = updated;
  }

  return ret;
}

void sp_rel(sp_t sp) {

  // if the target pointer was null, it was not reference counted
  if (sp.ptr == NULL) {
    assert(sp.impl == NULL && "null pointer with non-null control block");
    return;
  }

  assert(sp.impl != NULL && "non-null pointer with no control block");
  dec_ref(sp.impl);
}

void sp_store(asp_t *dst, sp_t src) {
  assert(dst != NULL);

  // Disclaimer: the justification for why the logic in this function is correct
  // is very subtle. It is extremely hard to understand this from the code
  // alone. Read the header comment in this file and study the referenced
  // presentations before trying to read this code.

  // swap in our new value
  const asp_impl_t new_impl = {.ctrl = src.impl};
  const dword_t new = impl2asp(new_impl);
  const dword_t old = dword_atomic_xchg(dst, new);

  const asp_impl_t old_impl = asp2impl(old);
  if (old_impl.ctrl != NULL) {
    // TODO: is it valid to fuse these two atomics?
    if (old_impl.load_count > 0)
      inc_load_ref(old_impl.ctrl, old_impl.load_count);
    DELAY3();
    // drop a reference for `dst` we just overwrote
    dec_ref(old_impl.ctrl);
  }
}
