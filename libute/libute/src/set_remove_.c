/// @file
/// @brief Implementation of set removal
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "set.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <ute/hash.h>
#include <ute/set.h>

bool set_remove_(set_t_ *set, const void *item, size_t item_size) {
  assert(set != NULL);
  assert(item != NULL || item_size == 0);

  const size_t h = hash(item, item_size);
  for (size_t i = 0; i < set->capacity; ++i) {
    const size_t index = (h + i) % set->capacity;
    const uintptr_t slot = set->base[index];

    // if this slot is unoccupied, we have probed as far as the item could be
    if (set_slot_is_free(slot))
      break;

    // skip tombstones
    if (set_slot_is_deleted(slot))
      continue;

    // is this our sought item?
    const void *const p = set_slot_to_ptr(slot);
    if (item_size == 0 || memcmp(item, p, item_size) == 0) {
      // mark as deleted
      set->base[index] = slot | 1;
      ++set->deleted;
      return true;
    }
  }

  return false;
}
