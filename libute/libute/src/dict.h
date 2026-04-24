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
#include <string.h>
#include <ute/asp.h>
#include <ute/dict.h>

/// internal implementation of a dictionary
///
/// A dictionary data structure looks like:
///
///   dict_t_     dict_impl_t
///   ┌───────┐   ┌──────────┐   ┌───────┬───────┬───────┬── key slots
///   │ root¹ ├──►│   key    ├──►│   0²  │   1²  │   2²  │ …
///   │       │   ├──────────┤   └───┬───┴───────┴───┬───┴──
///   └───────┘   │  value   ├┐      │               │
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
/// ² These are shared pointers, 2 words wide.
typedef struct {
  /// backing storage of dictionary key slots
  ///
  /// The key slots are shared pointers, made up of two words.
  atomic_dword_t *key;

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
  _Atomic uintptr_t *value;

  _Atomic size_t used; ///< how many key slots are non-empty?
  _Atomic size_t size; ///< how many value slots are non-empty?
  size_t capacity; ///< exponent + 1 of how many total slots at `key`/`value`?
} dict_impl_t;

/// get the capacity (in slots) of a dictionary
static inline size_t dict_capacity(const dict_impl_t dict) {
  return (size_t)1 << dict.capacity >> 1;
}

/// atomically read a key slot from a hash table
static inline dword_t key_slot_load(atomic_dword_t *slotptr) {
  return dword_atomic_load(slotptr);
}

/// atomically read a value slot from a hash table
static inline uintptr_t value_slot_load(_Atomic uintptr_t *slotptr) {
  return atomic_load_explicit(slotptr, memory_order_acquire);
}

/// atomically compare-and-swap into a hash table key slot
static inline bool key_slot_cas(atomic_dword_t *slotptr, dword_t *expected,
                                dword_t desired) {
  return dword_atomic_cas(slotptr, expected, desired);
}

/// atomically compare-and-swap into a hash table value slot
static inline bool value_slot_cas(_Atomic uintptr_t *slotptr,
                                  uintptr_t *expected, uintptr_t desired) {
  return atomic_compare_exchange_strong_explicit(
      slotptr, expected, desired, memory_order_acq_rel, memory_order_acquire);
}

/// mask for migration bit (see above)
enum { MIGRATED = (uintptr_t)1 };

/// deserialise a key slot back into its originating shared pointer
static inline sp_t key_slot_decode(dword_t slot) {
  sp_t decoded;
  assert(sizeof(slot) >= sizeof(decoded));
  memcpy(&decoded, &slot, sizeof(decoded));
  return decoded;
}

/// serialise a shared pointer into a key slot
static inline dword_t key_slot_encode(sp_t ptr) {
  dword_t encoded = 0;
  assert(sizeof(encoded) >= sizeof(ptr));
  memcpy(&encoded, &ptr, sizeof(ptr));
  return encoded;
}

/// convert a key slot to its originating pointer
static inline void *key_slot_to_ptr(dword_t slot) {
  sp_t decoded = key_slot_decode(slot);
  return decoded.ptr;
}

/// is this key slot unoccupied?
static inline bool key_slot_is_free(dword_t slot) {
  return key_slot_to_ptr(slot) == NULL;
}

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
