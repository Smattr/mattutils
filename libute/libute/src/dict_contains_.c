/// @file
/// @brief Implementation of dictionary existence check
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "dict.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <ute/asp.h>
#include <ute/dict.h>
#include <ute/hash.h>

bool dict_contains_(dict_t_ *dict, const void *key, dict_sig_t_ sig) {
  assert(dict != NULL);
  assert(key != NULL || sig.key_size == 0);

  const size_t h = (sig.hash != NULL ? sig.hash : hash)(key, sig.key_size);

  // acquire a reference to the dictionary
  sp_t sp = sp_acq(&dict->root);

  // if the dictionary is uninitialised, it is semantically empty
  if (sp.ptr == NULL)
    return false;

  dict_impl_t *const d = sp.ptr;

  for (size_t i = 0; i < dict_capacity(*d); ++i) {
    const size_t index = (h + i) % dict_capacity(*d);
    const dword_t slot = slot_load(&d->base[index]);

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
    if (sig.key_size == 0 || memcmp(key, p, sig.key_size) == 0) {
      sp_rel(sp);
      return true;
    }
  }

  sp_rel(sp);
  return false;
}
