/// @file
/// @brief Implementation of dictionary removal
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "dict.h"
#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ute/alloc_aligned.h>
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
    const dword_t k = key_slot_load(&d->key[index]);

    // if this slot is unoccupied, we have probed as far as the item could be
    if (key_slot_is_free(k))
      break;

    // is this our sought item?
    const void *const p = key_slot_to_ptr(k);
    if (sig.key_size != 0 && memcmp(p, key, sig.key_size) != 0)
      continue;

    // load the corresponding value slot
    uintptr_t v = value_slot_load(&d->value[index]);

  retry2:
    if (value_slot_is_moved(v)) {
      // someone is rehashing the dictionary into new storage
      sp_rel(sp);
      goto retry1;
    }

    // is this entry already deleted?
    if (value_slot_is_free(v))
      break;

    // mark as deleted
    if (!value_slot_cas(&d->value[index], &v, 0))
      goto retry2;
    (void)atomic_fetch_sub_explicit(&d->size, 1, memory_order_acq_rel);
    sp_rel(sp);
    {
      void *const value = value_slot_to_ptr(v);
      if (value != NULL && sig.value_dtor != NULL)
        sig.value_dtor(value);
      free_aligned(value);
    }
    return true;
  }

  sp_rel(sp);
  return false;
}
