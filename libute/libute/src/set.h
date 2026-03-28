/// @file
/// @brief Set internals
///
/// All content in this file is in the public domain. Use it any way you wish.

#pragma once

#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/// internal implementation of a set
///
/// A set data structure looks like:
///
///   set_t_      set_impl_t     slots
///   ┌───────┐   ┌──────────┐   ┌──────┬──────┬──────┬──
///   │ root¹ ├──►│   base   ├──►│  0   │  1   │  2   │ …
///   │       │   ├──────────┤   └──┬───┴──────┴──┬───┴──
///   └───────┘   │   used   │      │             │
///               ├──────────┤      ▼             ▼
///               │ deleted  │   ┌──────┐      ┌──────┐
///               ├──────────┤   │ item │      │ item │
///               │ capacity │   └──────┘      └──────┘
///               └──────────┘
///
/// `set_impl_t` carries no information about the size of set items. This is
/// expected to be passed in by callers.
typedef struct {
  /// backing storage of set slots
  ///
  /// The high bits of each slot are a pointer and the low bits indicate the
  /// state of the slot:
  ///
  ///    sizeof(uintptr_t) * CHAR_BIT   1 0
  ///    ▼                              ▼ ▼
  ///   ┌──────────────────────────────┬─┬─┐
  ///   └──────────────────────────────┴─┴─┘
  ///          pointer to item          ▲ ▲
  ///                                   │ │
  ///               has been migrated? ─┘ │
  ///                  has been deleted? ─┘
  _Atomic uintptr_t *base;

  _Atomic size_t used;    ///< how many slots are non-empty?
  _Atomic size_t deleted; ///< how many slots contain deleted items?
  size_t capacity;        ///< exponent + 1 of how many total slots at `base`?
} set_impl_t;

/// get the capacity (in slots) of a set
static inline size_t set_capacity(const set_impl_t set) {
  return (size_t)1 << set.capacity >> 1;
}

/// atomically read a slot from a hash table
static inline uintptr_t slot_load(_Atomic uintptr_t *slotptr) {
  return atomic_load_explicit(slotptr, memory_order_acquire);
}

/// atomically compare-and-swap into a hash table slot
static inline bool slot_cas(_Atomic uintptr_t *slotptr, uintptr_t *expected,
                            uintptr_t desired) {
  return atomic_compare_exchange_strong_explicit(
      slotptr, expected, desired, memory_order_acq_rel, memory_order_acquire);
}

enum {
  MIGRATED = (uintptr_t)2, ///< mask for migration bit (see above)
  DELETED = (uintptr_t)1,  ///< mask for deletion bit (see above)
};

/// is this set slot unoccupied?
static inline bool slot_is_free(uintptr_t slot) {
  return (slot & ~MIGRATED) == 0;
}

/// does this set slot contain an item that was deleted?
static inline bool slot_is_deleted(uintptr_t slot) {
  return (slot & DELETED) != 0;
}

/// derive the equivalent deleted representation of a slot
static inline uintptr_t slot_deleted(uintptr_t slot) {
  assert(!slot_is_deleted(slot));
  return slot | DELETED;
}

/// has this slot been migrated to a new set?
static inline bool slot_is_moved(uintptr_t slot) {
  return (slot & MIGRATED) != 0;
}

/// derive the equivalent moved representation of a slot
static inline uintptr_t slot_moved(uintptr_t slot) {
  assert(!slot_is_moved(slot));
  return slot | MIGRATED;
}

/// convert a set slot to its originating item pointer
static inline void *slot_to_ptr(uintptr_t slot) {
  return (void *)(slot & ~(MIGRATED | DELETED));
}

/// convert an item pointer to a set slot
static inline uintptr_t ptr_to_slot(void *ptr) {
  const uintptr_t slot = (uintptr_t)ptr;
  assert((slot & (MIGRATED | DELETED)) == 0 &&
         "item pointer insufficiently aligned");
  return slot;
}
