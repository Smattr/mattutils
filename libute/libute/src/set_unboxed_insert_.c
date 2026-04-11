/// @file
/// @brief Implementation of set insertion, for unboxed set
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "set_unboxed.h"
#include <assert.h>
#include <errno.h>
#include <stdalign.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <ute/asp.h>
#include <ute/attr.h>
#include <ute/hash.h>
#include <ute/set.h>

/// deallocate a set that is going out of scope
///
/// @param set Set to operate on
/// @param context Ignored
static void dtor(void *set, void *context UNUSED) {
  assert(set != NULL);

  set_impl_t *const s = set;

  free(s->base);
  free(s);
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
static int insert(set_impl_t *set, const void *item, set_sig_t_ sig) {
  assert(set != NULL);

  const size_t h = (sig.hash != NULL ? sig.hash : hash)(item, sig.size);
  for (size_t i = 0; i < set_capacity(*set); ++i) {
    const size_t index = (h + i) % set_capacity(*set);
    slot_t slot = slot_load(&set->base[index]);
  retry:

    // has someone else begun a migration?
    if (slot_is_moved(slot))
      return ENOMEM;

    // if this slot is unoccupied, try to insert our item
    if (slot_is_free(slot)) {
      if (!slot_cas(&set->base[index], &slot, ptr_to_slot(item, sig.size)))
        goto retry;
      (void)atomic_fetch_add_explicit(&set->used, 1, memory_order_acq_rel);
      return 0;
    }

    // if this is a deleted item, skip over it
    if (slot_is_deleted(slot))
      continue;

    // otherwise, check if this is our item already present
    void *const p = SLOT_TO_PTR(slot);
    if (eq(p, item, sig))
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
    slot_t slot = slot_load(&src->base[i]);
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

    const void *const p = SLOT_TO_PTR(slot);
    const int rc UNUSED = insert(dst, p, sig);
    assert(rc == 0 && "rehash destination not owned exclusively?");
  }

  return 0;
}

int set_unboxed_insert_(set_t_ *set, const void *item, set_sig_t_ sig) {
  assert(set != NULL);
  assert(item != NULL || sig.size == 0);
  assert(sig.size < sizeof(uintptr_t));
  assert(sig.alignment <= alignof(uintptr_t));
  assert(sig.dtor == NULL);

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
    _Atomic slot_t *const b = calloc((size_t)1 << c >> 1, sizeof(b[0]));
    if (b == NULL) {
      sp_rel(sp);
      return ENOMEM;
    }

    set_impl_t *const new = malloc(sizeof(*new));
    if (new == NULL) {
      free(b);
      sp_rel(sp);
      return ENOMEM;
    }
    *new = (set_impl_t){.base = b, .capacity = c};

    sp_t new_sp = sp_new(new, dtor, NULL);
    if (new_sp.ptr == NULL) {
      dtor(new, NULL);
      sp_rel(sp);
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

  // insert the new item
  {
    const int rc = insert(s, item, sig);
    sp_rel(sp);
    if (rc != 0) {
      if (rc != EEXIST)
        goto retry;
    }
  }

  return 0;
}
