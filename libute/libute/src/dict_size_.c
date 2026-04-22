/// @file
/// @brief Implementation of dictionary size
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "dict.h"
#include <assert.h>
#include <stdatomic.h>
#include <stddef.h>
#include <ute/asp.h>
#include <ute/dict.h>

size_t dict_size_(dict_t_ *dict) {
  assert(dict != NULL);

  // acquire a reference to the dictionary
  sp_t sp = sp_acq(&dict->root);

  // an uninitialised dictionary is semantically empty
  if (sp.ptr == NULL)
    return 0;

  const dict_impl_t *const d = sp.ptr;
  const size_t used = atomic_load_explicit(&d->used, memory_order_acquire);
  const size_t deleted =
      atomic_load_explicit(&d->deleted, memory_order_acquire);

  sp_rel(sp);

  // in the case of racing insertions and deletions, we can see an inconsistent
  // state
  if (used < deleted)
    return 0;

  return used - deleted;
}
