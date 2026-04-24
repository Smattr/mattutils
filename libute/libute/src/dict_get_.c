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
    const dword_t k = key_slot_load(&d->key[index]);

    // if this slot is unoccupied, we have probed as far as the item could be
    if (key_slot_is_free(k))
      break;

    // is this our sought item?
    void *const p = key_slot_to_ptr(k);
    if (sig.key_size != 0 && memcmp(p, key, sig.key_size) != 0)
      continue;

    // load the corresponding value slot
    const uintptr_t v = value_slot_load(&d->value[index]);

    sp_rel(sp);

    // we should not encounter anyone else rehashing the dictionary because this
    // violates our precondition
    assert(!value_slot_is_moved(v) && "race between DICT_GET and modifier");

    return value_slot_to_ptr(v);
  }

  sp_rel(sp);
  return NULL;
}
