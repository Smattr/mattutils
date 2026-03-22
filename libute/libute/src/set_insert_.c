/// @file
/// @brief Implementation of set insertion
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "set.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ute/attr.h>
#include <ute/hash.h>
#include <ute/set.h>

/// insert an item into a set
///
/// This function assumes the caller has made enough space that the insertion
/// can succeed without having to expand the set.
///
/// @param set Set to operate on
/// @param item Copy to insert
/// @param item_size Byte size of `item`
/// @return True if the item was inserted
static bool insert(set_t_ *set, void *item, size_t item_size) {
  assert(set != NULL);
  assert(set->used < set->capacity);
  assert(((uintptr_t)item & 1) == 0 && "heap pointer insufficiently aligned");

  const size_t h = hash(item, item_size);
  for (size_t i = 0; i < set->capacity; ++i) {
    const size_t index = (h + i) % set->capacity;
    const uintptr_t slot = set->base[index];

    // if this slot is unoccupied, try to insert our item
    if (slot_is_free(slot)) {
      set->base[index] = (uintptr_t)item;
      ++set->used;
      return true;
    }

    // if this is a deleted item, skip over it
    if (slot_is_deleted(slot))
      continue;

    // otherwise, check if this is our item already present
    void *const p = slot_to_ptr(slot);
    if (item_size == 0 || memcmp(p, item, item_size) == 0)
      return false;
  }
  UNREACHABLE();
}

/// insert everything from one set into another
///
/// The destination set is assumed to have enough space to store all items from
/// the source set without expansion. The source set is “consumed” in that the
/// destination takes ownership over all its items.
///
/// @param dst Set to insert into
/// @param src Set to insert from
/// @param item_size Byte size of set items
static void rehash(set_t_ *dst, set_t_ *src, size_t item_size) {
  assert(dst != NULL);
  assert(src != NULL);

  for (size_t i = 0; i < src->capacity; ++i) {
    const uintptr_t slot = src->base[i];
    if (slot_is_free(slot))
      continue;
    void *const p = slot_to_ptr(slot);
    if (slot_is_deleted(slot)) {
      free(p);
      continue;
    }
    const bool inserted UNUSED = insert(dst, p, item_size);
    assert(inserted);
  }
}

int set_insert_(set_t_ *set, const void *item, size_t item_size) {
  assert(set != NULL);
  assert(item != NULL || item_size == 0);

  // percentage occupancy at which we expand the backing storage
  enum { LOAD_FACTOR = 70 };

  // do we need to expand the backing storage?
  if (set->used * 100 >= set->capacity * LOAD_FACTOR) {
    const size_t c = set->capacity == 0 ? 1 : set->capacity * 2;
    uintptr_t *const b = calloc(c, sizeof(b[0]));
    if (b == NULL)
      return ENOMEM;

    set_t_ new = {.base = b, .capacity = c};
    rehash(&new, set, item_size);

    free(set->base);
    *set = new;
  }

  // copy the item for insertion
  void *const p = malloc(item_size == 0 ? 1 : item_size);
  if (p == NULL)
    return ENOMEM;
  if (item_size > 0)
    memcpy(p, item, item_size);

  // insert it
  const bool inserted = insert(set, p, item_size);
  if (!inserted)
    free(p);

  return 0;
}
