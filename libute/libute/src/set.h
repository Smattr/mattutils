/// @file
/// @brief Set internals
///
/// All content in this file is in the public domain. Use it any way you wish.

#pragma once

#include <ute/set.h>
#include <stdint.h>
#include <stdbool.h>

/// is this set slot unoccupied?
static inline bool slot_is_free(uintptr_t slot) {
  return slot == 0;
}

/// does this set slot contain an item that was deleted?
static inline bool slot_is_deleted(uintptr_t slot) {
  return slot & 1;
}

/// convert a set slot to its originating item pointer
static inline void *slot_to_ptr(uintptr_t slot) {
  return (void*)(slot & (uintptr_t)~1);
}
