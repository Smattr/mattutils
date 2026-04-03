/// @file
/// @brief Implementation of set removal, for unboxed set
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "set_unboxed.h"
#include <assert.h>
#include <stdalign.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <ute/asp.h>
#include <ute/hash.h>
#include <ute/set.h>

bool set_unboxed_remove_(set_t_ *set, const void *item, set_sig_t_ sig) {
  assert(set != NULL);
  assert(item != NULL || sig.size == 0);
  assert(sig.size < sizeof(uintptr_t));
  assert(sig.alignment <= alignof(uintptr_t));

  const size_t h = (sig.hash != NULL ? sig.hash : hash)(item, sig.size);

retry1:;
  // acquire a reference to the set
  sp_t sp = sp_acq(&set->root);

  // if the set is uninitialised, it is semantically empty
  if (sp.ptr == NULL)
    return false;

  set_impl_t *const s = sp.ptr;

  for (size_t i = 0; i < set_capacity(*s); ++i) {
    const size_t index = (h + i) % set_capacity(*s);
    slot_t slot = slot_load(&s->base[index]);

  retry2:
    if (slot_is_moved(slot)) {
      // someone is rehashing the set into new storage
      sp_rel(sp);
      goto retry1;
    }

    // if this slot is unoccupied, we have probed as far as the item could be
    if (slot_is_free(slot))
      break;

    // skip tombstones
    if (slot_is_deleted(slot))
      continue;

    // is this our sought item?
    const void *const p = SLOT_TO_PTR(slot);
    if (eq(item, p, sig)) {
      // mark as deleted
      if (!slot_cas(&s->base[index], &slot, slot_deleted(slot)))
        goto retry2;
      (void)atomic_fetch_add_explicit(&s->deleted, 1, memory_order_acq_rel);
      sp_rel(sp);
      return true;
    }
  }

  sp_rel(sp);
  return false;
}
