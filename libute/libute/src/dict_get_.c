/// @file
/// @brief Implementation of dictionary retrieval
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "dict.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <ute/asp.h>
#include <ute/dict.h>
#include <ute/dword.h>
#include <ute/hash.h>

void *dict_get_(dict_t_ *dict, const void *key, dict_sig_t_ sig) {
  assert(dict != NULL);
  assert(key != NULL || sig.key_size == 0);

  const size_t h = (sig.hash != NULL ? sig.hash : hash)(key, sig.key_size);

  // acquire a reference to the dictionary
  sp_t sp = sp_acq(&dict->root);

  // if the dictionary is uninitialised, it is semantically empty
  if (sp.ptr == NULL)
    return NULL;

  dict_impl_t *const d = sp.ptr;

  for (size_t i = 0; i < dict_capacity(*d); ++i) {
    const size_t index = (h + i) % dict_capacity(*d);
    dword_t slot = slot_load(&d->base[index]);

    // we should not encounter anyone else rehashing the dictionary because this
    // violates our precondition
    assert(!slot_is_moved(slot) && "race between DICT_GET and modifier");

    // if this slot is unoccupied, we have probed as far as the item could be
    if (slot_is_free(slot))
      break;

    // skip tombstones
    if (slot_is_deleted(slot))
      continue;

    // is this our sought item?
    void *const p = slot_to_ptr(slot);
    if (sig.key_size == 0 || memcmp(p, key, sig.key_size) == 0) {
      sp_rel(sp);
      // compute a pointer to the value within the slot
      void *const value = (void *)((uintptr_t)p + sig.witness_value_offset);
      return value;
    }
  }

  sp_rel(sp);
  return NULL;
}
