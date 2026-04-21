/// @file
/// @brief Implementation of dictionary removal
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "dict.h"
#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <ute/asp.h>
#include <ute/dict.h>
#include <ute/dword.h>
#include <ute/hash.h>

bool dict_remove_(dict_t_ *dict, const void *key, dict_sig_t_ sig) {
  assert(dict != NULL);
  assert(key != NULL || sig.key_size == 0);

  const size_t h = (sig.hash != NULL ? sig.hash : hash)(key, sig.key_size);

retry1:;
  // acquire a reference to the dictionary
  sp_t sp = sp_acq(&dict->root);

  // if the dictionary is uninitialised, it is semantically empty
  if (sp.ptr == NULL)
    return false;

  dict_impl_t *const d = sp.ptr;

  for (size_t i = 0; i < dict_capacity(*d); ++i) {
    const size_t index = (h + i) % dict_capacity(*d);
    dword_t slot = slot_load(&d->base[index]);

  retry2:
    if (slot_is_moved(slot)) {
      // someone is rehashing the dictionary into new storage
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
    const void *const p = slot_to_ptr(slot);
    if (sig.key_size == 0 || memcmp(p, key, sig.key_size) == 0) {
      // mark as deleted
      if (!slot_cas(&d->base[index], &slot, slot_deleted(slot)))
        goto retry2;
      (void)atomic_fetch_add_explicit(&d->deleted, 1, memory_order_acq_rel);
      sp_rel(sp);
      return true;
    }
  }

  sp_rel(sp);
  return false;
}
