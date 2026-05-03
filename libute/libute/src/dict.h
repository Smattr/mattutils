/// @file
/// @brief Dictionary internals
///
/// All content in this file is in the public domain. Use it any way you wish.

#pragma once

#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <ute/asp.h>
#include <ute/dict.h>

/// internal implementation of a dictionary
///
/// A dictionary data structure looks like:
///
///   dict_t_     dict_impl_t    ┌───────┬───────┬───────┬── ctrl slots
///   ┌───────┐   ┌──────────┐ ┌►│   0²  │   1²  │   2²  │ …
///   │ root¹ ├──►│   ctrl   ├─┘ └───────┴───────┴───────┴──
///   │       │   ├──────────┤
///   └───────┘   │   key    ├─┐ ┌───────┬───────┬───────┬── key slots
///               ├──────────┤ └►│   0²  │   1²  │   2²  │ …
///               │  value   ├┐  └───┬───┴───────┴───┬───┴──
///               ├──────────┤│      ▼               ▼
///               │   used   ││  ┌───────┐       ┌───────┐
///               ├──────────┤│  │  key  │       │  key  │
///               │   size   ││  └───────┘       └───────┘
///               ├──────────┤│  ┌───────┬───────┬───────┬── value slots
///               │ capacity │└─►│   0   │   1   │   2   │ …
///               └──────────┘   └───┬───┴───────┴───┬───┴──
///                                  │               │
///                                  ▼               ▼
///                              ┌───────┐       ┌───────┐
///                              │ value │       │ value │
///                              └───────┘       └───────┘
///
/// `dict_impl_t` carries no information about the size of dictionary keys or
/// values. This is expected to be passed in by callers.
///
/// ¹ This is an atomic shared pointer, 2 words wide.
/// ² These are the two halves of a shared pointer,
///   `(sp_t){.ptr = key[i], .impl = ctrl[i]}`.
typedef struct {
  /// backing storage for the dictionary keys’ metadata
  sp_ctrl_t *_Atomic *ctrl;

  /// backing storage of dictionary keys
  void *_Atomic *key;

  /// backing storage of dictionary value slots
  ///
  /// The high bits of each slot’s word are the actual pointer and the low bits
  /// indicate the state of the slot:
  ///
  ///              ┌─ sizeof(uintptr_t) * CHAR_BIT - 1
  ///              │                                0
  ///              ▼                                ▼
  ///   value[i]: ┌────────────────────────────────┬─┐
  ///             └────────────────────────────────┴─┘
  ///                     pointer to value          ▲
  ///                                               │
  ///                                               │
  ///                           has been migrated? ─┘
  atomic_uintptr_t *value;

  atomic_size_t used; ///< how many key slots are non-empty?
  atomic_size_t size; ///< how many value slots are non-empty?
  size_t capacity; ///< exponent + 1 of how many total slots at `key`/`value`?
} dict_impl_t;

/// get the capacity (in slots) of a dictionary
static inline size_t dict_capacity(const dict_impl_t dict) {
  return (size_t)1 << dict.capacity >> 1;
}

/// atomically read a control pointer
static inline sp_ctrl_t *ctrl_load(sp_ctrl_t *_Atomic *src) {
  return atomic_load_explicit(src, memory_order_acquire);
}

/// atomically read a key pointer
static inline void *key_load(void *_Atomic *src) {
  return atomic_load_explicit(src, memory_order_acquire);
}

/// atomically read a value slot
static inline uintptr_t value_slot_load(atomic_uintptr_t *slotptr) {
  return atomic_load_explicit(slotptr, memory_order_acquire);
}

/// atomically compare-and-swap into a control pointer
static inline bool ctrl_cas(sp_ctrl_t *_Atomic *dst, sp_ctrl_t **expected,
                            sp_ctrl_t *desired) {
  assert(expected != NULL);
  assert(*expected == NULL && "overwriting non-empty control slot");
  assert(desired != NULL);
  return atomic_compare_exchange_strong_explicit(
      dst, expected, desired, memory_order_acq_rel, memory_order_acquire);
}

/// atomically write to an empty key pointer
static inline void key_store(void *_Atomic *dst, void *src) {
  assert(atomic_load_explicit(dst, memory_order_acquire) == NULL &&
         "overwriting non-empty key slot");
  atomic_store_explicit(dst, src, memory_order_release);
}

/// atomically compare-and-swap into a hash table value slot
static inline bool value_slot_cas(atomic_uintptr_t *slotptr,
                                  uintptr_t *expected, uintptr_t desired) {
  return atomic_compare_exchange_strong_explicit(
      slotptr, expected, desired, memory_order_acq_rel, memory_order_acquire);
}

/// mask for migration bit (see above)
enum { MIGRATED = (uintptr_t)1 };

/// has this slot been migrated to a new dictionary?
static inline bool value_slot_is_moved(uintptr_t slot) {
  return (slot & MIGRATED) != 0;
}

/// derive the equivalent moved representation of a value slot
static inline dword_t value_slot_moved(uintptr_t slot) {
  assert(!value_slot_is_moved(slot));
  return slot | MIGRATED;
}

/// convert a value slot to its originating pointer
static inline void *value_slot_to_ptr(uintptr_t slot) {
  return (void *)(slot & ~MIGRATED);
}

/// is this value slot unoccupied?
static inline bool value_slot_is_free(uintptr_t slot) {
  return value_slot_to_ptr(slot) == NULL;
}
