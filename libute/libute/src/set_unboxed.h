/// @file
/// @brief Unboxed set internals
///
/// The set implemented below stores its elements inline in the set slots. It
/// can only handle items that are smaller than `uintptr_t` and the same or
/// weaker aligned.
///
/// All content in this file is in the public domain. Use it any way you wish.

#pragma once

#include <assert.h>
#include <limits.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <ute/set.h>

/// a set slot (see below)
#ifdef _MSC_VER
typedef uintptr_t slot_t;
#else
typedef uintptr_t __attribute__((may_alias)) slot_t;
#endif

/// internal implementation of a set
///
/// A set data structure looks like:
///
///   set_t_      set_impl_t     slots
///   ┌───────┐   ┌──────────┐   ┌──────┬──────┬──────┬──
///   │ root¹ ├──►│   base   ├──►│ item │      │ item │ …
///   │       │   ├──────────┤   └──────┴──────┴──────┴──
///   └───────┘   │   used   │
///               ├──────────┤
///               │ deleted  │
///               ├──────────┤
///               │ capacity │
///               └──────────┘
///
/// `set_impl_t` carries no information about the size of set items. This is
/// expected to be passed in by callers.
///
/// ¹ This is an atomic shared pointer, 2 words wide.
typedef struct {
  /// backing storage of set slots
  ///
  /// The low bits of each slot are the item value and the high bits indicate
  /// the state of the slot:
  ///
  ///    ┌── sizeof(uintptr_t) * CHAR_BIT - 1
  ///    │ ┌── sizeof(uintptr_t) * CHAR_BIT - 2
  ///    │ │ ┌── sizeof(uintptr_t) * CHAR_BIT - 3
  ///    │ │ │                            0
  ///    ▼ ▼ ▼                            ▼
  ///   ┌─┬─┬─┬────────────────────────────┐
  ///   └─┴─┴─┴────────────────────────────┘
  ///    ▲ ▲ ▲            item
  ///    │ │ │
  ///    │ │ └── slot contains an item?
  ///    │ └── has been migrated?
  ///    └── has been deleted?
  _Atomic slot_t *base;

  _Atomic size_t used;    ///< how many slots are non-empty?
  _Atomic size_t deleted; ///< how many slots contain deleted items?
  size_t capacity;        ///< exponent + 1 of how many total slots at `base`?
} set_impl_t;

/// get the capacity (in slots) of a set
static inline size_t set_capacity(const set_impl_t set) {
  return (size_t)1 << set.capacity >> 1;
}

/// atomically read a slot from a hash table
static inline slot_t slot_load(_Atomic slot_t *slotptr) {
  return atomic_load_explicit(slotptr, memory_order_acquire);
}

/// atomically compare-and-swap into a hash table slot
static inline bool slot_cas(_Atomic slot_t *slotptr, slot_t *expected,
                            slot_t desired) {
  return atomic_compare_exchange_strong_explicit(
      slotptr, expected, desired, memory_order_acq_rel, memory_order_acquire);
}

enum {
  /// mask for present bit (see above)
  PRESENT = (uintptr_t)1 << (sizeof(uintptr_t) * CHAR_BIT - 3),
  /// mask for migration bit (see above)
  MIGRATED = (uintptr_t)1 << (sizeof(uintptr_t) * CHAR_BIT - 2),
  /// mask for deletion bit (see above)
  DELETED = (uintptr_t)1 << (sizeof(uintptr_t) * CHAR_BIT - 1),
};

/// is this set slot unoccupied?
static inline bool slot_is_free(slot_t slot) { return (slot & ~MIGRATED) == 0; }

/// does this set slot contain an item that was deleted?
static inline bool slot_is_deleted(slot_t slot) {
  return (slot & DELETED) != 0;
}

/// derive the equivalent deleted representation of a slot
static inline slot_t slot_deleted(slot_t slot) {
  assert(!slot_is_deleted(slot));
  return slot | DELETED;
}

/// has this slot been migrated to a new set?
static inline bool slot_is_moved(slot_t slot) { return (slot & MIGRATED) != 0; }

/// derive the equivalent moved representation of a slot
static inline slot_t slot_moved(slot_t slot) {
  assert(!slot_is_moved(slot));
  return slot | MIGRATED;
}

/// convert a set slot to its originating item pointer
#define SLOT_TO_PTR(slot) (&(slot))

/// convert an item pointer to a set slot
static inline slot_t ptr_to_slot(const void *ptr, size_t size) {
  assert(ptr != NULL || size == 0);
  assert(size < sizeof(uintptr_t));

  slot_t slot = 0;
  if (size > 0)
    memcpy(&slot, ptr, size);
  return slot | PRESENT;
}

/// are two set items equal?
static inline bool eq(const void *a, const void *b, set_sig_t_ sig) {
  assert(a != NULL || sig.size == 0);
  assert(b != NULL || sig.size == 0);

  if (sig.size == 0)
    return true;
  if (sig.eq != NULL)
    return sig.eq(a, b, sig.size);
  return memcmp(a, b, sig.size) == 0;
}
