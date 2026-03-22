/// @file
/// @brief Implementation of set destruction
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "set.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <ute/set.h>

void set_free_(set_t_ *set) {
  assert(set != NULL);

  // deallocate all items (deleted or not) in the set
  for (size_t i = 0; i < set->capacity; ++i) {
    const uintptr_t slot = set->base[i];
    if (set_slot_is_free(slot))
      continue;
    void *const p = set_slot_to_ptr(slot);
    free(p);
  }

  free(set->base);
  *set = (set_t_){0};
}
