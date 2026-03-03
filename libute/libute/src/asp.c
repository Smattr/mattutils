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
#include <stdlib.h>
#include <string.h>
#include <ute/asp.h>
#include <ute/dword.h>

struct sp_ctrl {
  void *value;              ///< the managed underlying pointer
  void (*dtor)(void *);     ///< optional user-supplied destructor
  _Atomic size_t ref_count; ///< number of outstanding references
};

/// increment the reference count of a shared pointer
///
/// @param ctrl Control block to operate on
/// @param by Number of reference to add
static void inc_ref(sp_ctrl_t *ctrl, size_t by) {
  assert(ctrl != NULL);
  (void)atomic_fetch_add_explicit(&ctrl->ref_count, by, memory_order_acq_rel);
}

/// decrement the reference count of a shared pointer by 1
///
/// @param ctrl Control block to operate on
static void dec_ref(sp_ctrl_t *ctrl) {
  assert(ctrl != NULL);

  const size_t old =
      atomic_fetch_sub_explicit(&ctrl->ref_count, 1, memory_order_acq_rel);
  assert(old > 0 && "dropping a reference that was not held");

  // if we just dropped the last reference, clean up
  if (old == 1) {
    if (ctrl->dtor != NULL) {
      ctrl->dtor(ctrl->value);
    }
    free(ctrl);
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
      dec_ref(impl.ctrl);
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
      inc_ref(old_impl.ctrl, old_impl.load_count);
    // drop a reference for `dst` we just overwrote
    dec_ref(old_impl.ctrl);
  }
}
