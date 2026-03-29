/// @file
/// @brief Implementation of set size, for unboxed set
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "set.h"
#include "set_unboxed.h"
#include <assert.h>
#include <stdatomic.h>
#include <stddef.h>
#include <ute/asp.h>
#include <ute/set.h>

size_t set_unboxed_size(set_t_ *set) {
  assert(set != NULL);

  // acquire a reference to the set
  sp_t sp = sp_acq(&set->root);

  // an uninitialised set is semantically empty
  if (sp.ptr == NULL)
    return 0;

  set_impl_t *const s = sp.ptr;
  const size_t used = atomic_load_explicit(&s->used, memory_order_acquire);
  const size_t deleted =
      atomic_load_explicit(&s->deleted, memory_order_acquire);

  sp_rel(sp);

  // in the case of racing insertions and deletes, we can see an inconsistent
  // state
  if (used < deleted)
    return 0;

  return used - deleted;
}
