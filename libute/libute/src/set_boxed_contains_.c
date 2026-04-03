/// @file
/// @brief Implementation of set existence check, for boxed set
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "set_boxed.h"
#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <ute/asp.h>
#include <ute/hash.h>
#include <ute/set.h>

bool set_boxed_contains_(set_t_ *set, const void *item, set_sig_t_ sig) {
  assert(set != NULL);
  assert(item != NULL || sig.size == 0);

  const size_t h = (sig.hash != NULL ? sig.hash : hash)(item, sig.size);

  // acquire a reference to the set
  sp_t sp = sp_acq(&set->root);

  // if the set is uninitialised, it is semantically empty
  if (sp.ptr == NULL)
    return false;

  set_impl_t *const s = sp.ptr;

  for (size_t i = 0; i < set_capacity(*s); ++i) {
    const size_t index = (h + i) % set_capacity(*s);
    const uintptr_t slot = slot_load(&s->base[index]);

    // skip checking whether this slot is moved or not, because we do not care
    // if we are racing with a rehashing and reading an older stale copy of the
    // table

    // if we see an empty slot, we have probed as far as this item would be
    if (slot_is_free(slot))
      break;

    // skip tombstones
    if (slot_is_deleted(slot))
      continue;

    // check if this is the item we are seeking
    const void *const p = slot_to_ptr(slot);
    if (eq(item, p, sig)) {
      sp_rel(sp);
      return true;
    }
  }

  sp_rel(sp);
  return false;
}
