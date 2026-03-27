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
/// This load count can also underflow (below 0) during correct operation. The
/// reasoning for why this underflow is allowed and why propagating this
/// underflowed value (that can then lead to a reference count _overflow_) is
/// correct is subtle. Again, see the commit history of this file.
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
///
/// All content in this file is in the public domain. Use it any way you wish.

#include <assert.h>
#include <limits.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ute/asp.h>
#include <ute/attr.h>
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

/// factor for separating reference count and load count
///
/// The reference count for a shared pointer (`sp_ctrl_t.ref_count`) is actually
/// split into two “fields”:
///
///   0                                            sizeof(size_t) * CHAR_BIT - 1
///   ├─────────────────────────────┬─────────────────────────────┤
///   │ outstanding shared pointers │      propagated loads       │
///   └─────────────────────────────┴─────────────────────────────┘
///
/// `LOAD_SCALE` is the factor we need to multiple a count by for it to land in
/// the upper half:
///
///   ┌─────────────────────────────┬─────────────────────────────┐
///   │ 00000…                      │ 10000…                      │
///   └─────────────────────────────┴─────────────────────────────┘
static const size_t LOAD_SCALE = (size_t)1 << (sizeof(size_t) * CHAR_BIT / 2);

/// AND mask for extracting lower half of `sp_ctrl_t.ref_count`
static const size_t REFS_MASK UNUSED = LOAD_SCALE - 1;

struct sp_ctrl {
  void *value;                  ///< the managed underlying pointer
  void (*dtor)(void *, void *); ///< optional user-supplied destructor
  void *dtor_context;           ///< second parameter to `dtor`
  _Atomic size_t ref_count;     ///< outstanding references
};

/// increment the reference count of a shared pointer
///
/// @param ctrl Control block to operate on
/// @param by Number of references to add
static void inc_ref(sp_ctrl_t *ctrl, size_t by) {
  assert(ctrl != NULL);
  assert(by > 0 && "redundant inc_ref");
  assert(by < LOAD_SCALE && "overflow");

  (void)atomic_fetch_add_explicit(&ctrl->ref_count, by, memory_order_acq_rel);
}

/// propagate an increment of the load count of a shared pointer
///
/// @param ctrl Control block to operate on
/// @param by Number of loads to add
UNUSED static void inc_load_ref(sp_ctrl_t *ctrl, size_t by) {
  assert(ctrl != NULL);
  assert(by > 0 && "redundant inc_load_ref");

  const size_t old __attribute__((unused)) = atomic_fetch_add_explicit(
      &ctrl->ref_count, by * LOAD_SCALE, memory_order_acq_rel);
  assert((old & REFS_MASK) > 0 &&
         "changing load count while not holding a reference");
}

/// decrement the reference count of a shared pointer by 1
///
/// @param ctrl Control block to operate on
static void dec_ref(sp_ctrl_t *ctrl) {
  assert(ctrl != NULL);

  const size_t old =
      atomic_fetch_sub_explicit(&ctrl->ref_count, 1, memory_order_acq_rel);
  assert((old & REFS_MASK) > 0 && "dropping a reference that was not held");

  // if we just dropped the last reference, clean up
  if (old == 1) {
    if (ctrl->dtor != NULL)
      ctrl->dtor(ctrl->value, ctrl->dtor_context);
    free(ctrl);
  }
}

/// decrement the propagated load count of a shared pointer by 1
///
/// @param ctrl Control block to operate on
static void dec_load_ref(sp_ctrl_t *ctrl) {
  assert(ctrl != NULL);

  const size_t old __attribute__((unused)) = atomic_fetch_sub_explicit(
      &ctrl->ref_count, LOAD_SCALE, memory_order_acq_rel);
  assert((old & REFS_MASK) > 0 &&
         "changing load count while not holding a reference");
}

/// propagate an increment of the load count and decrement the ref count
///
/// This essentially performs a fused:
///
///   inc_load_ref(ctrl, load_by);
///   dec_ref(ctrl);
///
/// @param ctrl Control block to operate on
/// @param load_by Loads to propagate to the load count
UNUSED static void inc_and_dec(sp_ctrl_t *ctrl, size_t load_by) {
  assert(ctrl != NULL);
  assert(load_by > 0 && "redundant inc_and_dec");

  // combine an increment by `load_by` of the load count and a decrement of the
  // ref count
  const size_t addend = load_by * LOAD_SCALE - 1;

  const size_t old =
      atomic_fetch_add_explicit(&ctrl->ref_count, addend, memory_order_acq_rel);
  assert((old & REFS_MASK) > 0 && "dropping a reference that was not held");

  // if we just dropped the last reference, clean up
  if (old + load_by * LOAD_SCALE == 1) {
    if (ctrl->dtor != NULL)
      ctrl->dtor(ctrl->value, ctrl->dtor_context);
    free(ctrl);
  }
}

/// exposed implementation of an atomic shared pointer
///
/// External callers see atomic shared pointers as `atomic_dword_t`s. But we
/// operate on them as `asp_impl_t`s.
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

sp_t sp_new(void *value, void (*dtor)(void *, void *), void *dtor_context) {

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
  ctrl->dtor_context = dtor_context;
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
#if 1
    // as an optimisation, we can fuse what would otherwise be two separate
    // atomics in the other branch of this #if
    if (old_impl.load_count * LOAD_SCALE == 0) {
      // drop a reference for `dst` we just overwrote
      dec_ref(old_impl.ctrl);
    } else {
      // propagate load count while also dropping a reference for `dst`
      inc_and_dec(old_impl.ctrl, old_impl.load_count);
    }
#else
    if (old_impl.load_count * LOAD_SCALE != 0)
      inc_load_ref(old_impl.ctrl, old_impl.load_count);
    DELAY3();
    // drop a reference for `dst` we just overwrote
    dec_ref(old_impl.ctrl);
#endif
  }
}

bool sp_cas(asp_t *dst, sp_t expected, sp_t desired) {
  assert(dst != NULL);

  const asp_impl_t new_impl = {.ctrl = desired.impl};
  const dword_t new = impl2asp(new_impl);
  bool ret;

  for (asp_impl_t old_impl = {.ctrl = expected.impl};;) {
    dword_t old = impl2asp(old_impl);

    // try to swap in our desired value
    ret = dword_atomic_cas(dst, &old, new);
    old_impl = asp2impl(old);

    if (ret) {
      // replicate load propagation logic from `sp_store`
      if (old_impl.ctrl != NULL) {
#if 1
        // as an optimisation, we can fuse what would otherwise be two separate
        // atomics in the other branch of this #if
        if (old_impl.load_count * LOAD_SCALE == 0) {
          // drop a reference for `dst` we just overwrote
          dec_ref(old_impl.ctrl);
        } else {
          // propagate load count while also dropping a reference for `dst`
          inc_and_dec(old_impl.ctrl, old_impl.load_count);
        }
#else
        if (old_impl.load_count * LOAD_SCALE != 0)
          inc_load_ref(old_impl.ctrl, old_impl.load_count);
        DELAY3();
        // drop a reference for `dst` we just overwrote
        dec_ref(old_impl.ctrl);
#endif
      }
      break;
    }

    // does `dst` hold a different pointer than `expected`?
    if (old_impl.ctrl != expected.impl)
      break;
  }

  return ret;
}
