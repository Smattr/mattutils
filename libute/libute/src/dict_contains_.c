/// @file
/// @brief Implementation of dictionary existence check
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "dict.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <ute/asp.h>
#include <ute/dict.h>
#include <ute/hash.h>

bool dict_contains_(dict_t_ *dict, const void *key, dict_sig_t_ sig) {
  assert(dict != NULL);
  assert(key != NULL || sig.key_size == 0);

  const size_t h = (sig.hash != NULL ? sig.hash : hash)(key, sig.key_size);

retry:;
  // acquire a reference to the dictionary
  sp_t sp = sp_acq(&dict->root);

  // if the dictionary is uninitialised, it is semantically empty
  if (sp.ptr == NULL)
    return false;

  dict_impl_t *const d = sp.ptr;

  for (size_t i = 0; i < dict_capacity(*d); ++i) {
    const size_t index = (h + i) % dict_capacity(*d);
    const dword_t k = key_slot_load(&d->key[index]);

    // if we see an empty slot, we have probed as far as this item would be
    if (key_slot_is_free(k))
      break;

    // if this our sought item?
    const void *const p = key_slot_to_ptr(k);
    if (sig.key_size != 0 && memcmp(key, p, sig.key_size) != 0)
      continue;

    // load the corresponding value slot
    const uintptr_t v = value_slot_load(&d->value[index]);

    sp_rel(sp);

    if (value_slot_is_moved(v)) {
      // someone is rehashing the dictionary into new storage
      goto retry;
    }

    return !value_slot_is_free(v);
  }

  sp_rel(sp);
  return false;
}
