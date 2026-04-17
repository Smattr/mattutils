/// @file
/// @brief Implementation of set insertion, for boxed set
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "set_boxed.h"
#include <assert.h>
#include <errno.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ute/asp.h>
#include <ute/attr.h>
#include <ute/dword.h>
#include <ute/hash.h>
#include <ute/set.h>

/// `aligned_alloc` equivalent of `calloc`
static void *aligned_calloc(size_t alignment, size_t n, size_t size) {
  if (n > 0 && SIZE_MAX / n < size)
    return NULL;
  void *const p = aligned_alloc(alignment, n * size);
  if (p != NULL)
    memset(p, 0, n * size);
  return p;
}

/// allocate storage for a new set item
///
/// @param alignment Required alignment for the to-be-stored value
/// @param size Required size in bytes
/// @return Pointer to new storage on success or `NULL` on out of memory
static void *alloc(size_t alignment, size_t size) {

  // ensure we never return null for a successful allocation
  if (size == 0)
    size = 1;

  // For small (in both size and alignment) allocations, `aligned_alloc` may
  // delegate to `malloc`. In this scenario, some allocators will return
  // pointers that are < 4 byte aligned, which also appears to be explicitly
  // allowed under ≥C23. We need ≥ 4 byte alignment (see ./set.h), so be
  // explicit about this.
  // See also https://github.com/openjdk/jdk/pull/28235.
  if (alignment < 4) {
    alignment = 4;
    if (size % 4 != 0)
      size += 4 - size % 4;
  }

  return aligned_alloc(alignment, size);
}

/// deallocate a set that is going out of scope
///
/// @param set Set to operate on
/// @param context Ignored
static void set_dtor(void *set, void *context UNUSED) {
  assert(set != NULL);

  set_impl_t *const s = set;

  // free our slots
  for (size_t i = 0; i < set_capacity(*s); ++i) {
    const dword_t slot = slot_load(&s->base[i]);
    sp_t sp = slot_decode(slot);
    sp.ptr = slot_to_ptr(slot); // clear MIGRATED|DELETED
    sp_rel(sp);
  }

  free(s->base);
  free(s);
}

/// run the optional user-supplied destructor on a set element
///
/// @param ptr Pointer to the element
/// @param context Optional user-supplied destructor
static void slot_dtor_core(void *ptr, void *context) {
  assert(ptr != NULL);

  void (*user_dtor)(void *) = context;
  if (user_dtor != NULL && ptr != NULL)
    user_dtor(ptr);
}

/// deallocate a set element that is going out of scope
///
/// @param ptr Pointer to the element
/// @param context Optional user-supplied destructor
static void slot_dtor(void *ptr, void *context) {
  assert(ptr != NULL);

  slot_dtor_core(ptr, context);
  free(ptr);
}

/// insert an item into a set
///
/// The return value means:
///   • 0 – the item was inserted
///   • `EEXIST` – the item was already present
///   • `ENOMEM` – not enough space to insert the item
///
/// @param set Set to operate on
/// @param item Copy to insert
/// @param sig Signature of the set item type
/// @return 0 on success or an errno otherwise
static int insert(set_impl_t *set, sp_t item, set_sig_t_ sig) {
  assert(set != NULL);

  const size_t h = (sig.hash != NULL ? sig.hash : hash)(item.ptr, sig.size);
  for (size_t i = 0; i < set_capacity(*set); ++i) {
    const size_t index = (h + i) % set_capacity(*set);
    dword_t slot = slot_load(&set->base[index]);
  retry:

    // has someone else begun a migration?
    if (slot_is_moved(slot))
      return ENOMEM;

    // if this slot is unoccupied, try to insert our item
    if (slot_is_free(slot)) {
      if (!slot_cas(&set->base[index], &slot, slot_encode(item)))
        goto retry;
      (void)atomic_fetch_add_explicit(&set->used, 1, memory_order_acq_rel);
      return 0;
    }

    // if this is a deleted item, skip over it
    if (slot_is_deleted(slot))
      continue;

    // otherwise, check if this is our item already present
    void *const p = slot_to_ptr(slot);
    if (eq(p, item.ptr, sig))
      return EEXIST;
  }

  return ENOMEM;
}

/// insert everything from one set into another
///
/// The destination set is assumed to have enough space to store all items from
/// the source set without expansion. On success, the source set is “consumed”
/// in that the destination takes ownership over all its items.
///
/// @param dst Set to insert into
/// @param src Set to insert from
/// @param sig Signature of the set item type
/// @return 0 on success or an errno on failure
static int rehash(set_impl_t *dst, set_impl_t *src, set_sig_t_ sig) {
  assert(dst != NULL);
  assert(src == NULL || set_capacity(*dst) >= set_capacity(*src));

  // nothing to do for an uninitialised set
  if (src == NULL)
    return 0;

  for (size_t i = 0; i < set_capacity(*src); ++i) {
    dword_t slot = slot_load(&src->base[i]);
  retry:

    // Did someone else beat us to migration? CASing in the “migrated” bit to
    // the first slot is how we authoritatively claim that we and only we are
    // migrating this set, so we should only ever race with other migrators on
    // the first slot.
    if (slot_is_moved(slot)) {
      assert(i == 0 && "another migrator skipped the first slot");
      return EALREADY;
    }

    if (!slot_cas(&src->base[i], &slot, slot_moved(slot))) {
      // an inserter or deleter (or migrator if i == 0) beat us
      goto retry;
    }

    if (slot_is_free(slot) || slot_is_deleted(slot))
      continue;

    const sp_t item = slot_decode(slot);
    const sp_t copy = sp_dup(item);
    const int rc UNUSED = insert(dst, copy, sig);
    assert(rc == 0 && "rehash destination not owned exclusively?");
  }

  return 0;
}

int set_boxed_insert_(set_t_ *set, void *item, bool *exists, set_sig_t_ sig) {
  assert(set != NULL);
  assert(item != NULL || sig.size == 0);

  // copy the item for insertion
  void *const item_copy = alloc(sig.alignment, sig.size);
  if (item_copy == NULL) {
    slot_dtor_core(item, sig.dtor);
    return ENOMEM;
  }
  if (sig.size > 0)
    memcpy(item_copy, item, sig.size);
  const sp_t copy = sp_new(item_copy, slot_dtor, sig.dtor);
  if (copy.ptr == NULL) {
    slot_dtor(item_copy, sig.dtor);
    return ENOMEM;
  }

  // percentage occupancy at which we expand the backing storage
  enum { LOAD_FACTOR = 70 };

retry:;

  // acquire a reference to the set
  sp_t sp = sp_acq(&set->root);

  set_impl_t *const s = sp.ptr;
  const size_t used =
      s == NULL ? 0 : atomic_load_explicit(&s->used, memory_order_acquire);
  const size_t capacity = s == NULL ? 0 : set_capacity(*s);

  // do we need to expand the backing storage?
  if (used * 100 >= capacity * LOAD_FACTOR) {
    const size_t c = capacity + 1;
    atomic_dword_t *const b = aligned_calloc(alignof(atomic_dword_t),
                                             (size_t)1 << c >> 1, sizeof(b[0]));
    if (b == NULL) {
      sp_rel(sp);
      sp_rel(copy);
      return ENOMEM;
    }

    set_impl_t *const new = malloc(sizeof(*new));
    if (new == NULL) {
      free(b);
      sp_rel(sp);
      sp_rel(copy);
      return ENOMEM;
    }
    *new = (set_impl_t){.base = b, .capacity = c};

    sp_t new_sp = sp_new(new, set_dtor, NULL);
    if (new_sp.ptr == NULL) {
      set_dtor(new, NULL);
      sp_rel(sp);
      sp_rel(copy);
      return ENOMEM;
    }

    if (rehash(new, s, sig) != 0) {
      sp_rel(new_sp);
      sp_rel(sp);
      goto retry;
    }

    if (!sp_cas(&set->root, sp, new_sp))
      sp_rel(new_sp);
    sp_rel(sp);
    goto retry;
  }

  // insert it
  {
    const int rc = insert(s, copy, sig);
    sp_rel(sp);
    if (rc != 0) {
      if (rc != EEXIST)
        goto retry;
      sp_rel(copy);
    }
    if (exists != NULL)
      *exists = rc == EEXIST;
  }

  return 0;
}
