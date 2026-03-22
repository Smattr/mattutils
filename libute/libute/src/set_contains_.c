/// @file
/// @brief Implementation of set existence check
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

bool set_contains_(const set_t_ *set, const void *item, size_t item_size) {
  assert(set != NULL);
  assert(item != NULL || item_size == 0);

  const size_t h = hash(item, item_size);
  for (size_t i = 0; i < set->capacity; ++i) {
    const size_t index = (h + i) % set->capacity;
    const uintptr_t slot = set->base[index];

    // if we see an empty slot, we have probed as far as this item would be
    if (slot_is_free(slot))
      break;

    // skip tombstones
    if (slot_is_deleted(slot))
      continue;

    // check if this is the item we are seeking
    const void *const p = slot_to_ptr(slot);
    if (item_size == 0 || memcmp(item, p, item_size) == 0)
      return true;
  }

  return false;
}
