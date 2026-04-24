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
  const size_t size = atomic_load_explicit(&d->size, memory_order_acquire);

  sp_rel(sp);

  return size;
}
