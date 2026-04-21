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
///   dict_t_     dict_impl_t    slots
///   ┌───────┐   ┌──────────┐   ┌───────┬───────┬───────┬──
///   │ root¹ ├──►│   base   ├──►│   0   │   1   │   2   │ …
///   │       │   ├──────────┤   └───┬───┴───────┴───┬───┴──
///   └───────┘   │   used   │       │               │
///               ├──────────┤       ▼               ▼
///               │ deleted  │   ┌───────┐       ┌───────┐
///               ├──────────┤   │  key  │       │  key  │
///               │ capacity │   ├───────┤       ├───────┤
///               └──────────┘   │ value │       │ value │
///                              └───────┘       └───────┘
///
/// `dict_impl_t` carries no information about the size of dictionary items.
/// This is expected to be passed in by callers.
///
/// ¹ This is an atomic shared pointer, 2 words wide.
typedef struct {
  /// backing storage of dictionary slots
  ///
  /// The slots are shared pointers, made up of two words. The high bits of each
  /// slot’s `ptr` field are the actual raw pointer and the low bits indicate
  /// the state of the slot:
  ///
  ///                 ┌─ sizeof(uintptr_t) * CHAR_BIT - 1
  ///                 │                              1 0
  ///                 ▼                              ▼ ▼
  ///   base[i].ptr: ┌──────────────────────────────┬─┬─┐
  ///                └──────────────────────────────┴─┴─┘
  ///                       pointer to key+value     ▲ ▲
  ///                                                │ │
  ///                            has been migrated? ─┘ │
  ///                               has been deleted? ─┘
  atomic_dword_t *base;

  _Atomic size_t used;    ///< how many slots are non-empty?
  _Atomic size_t deleted; ///< how many slots contain deleted items?
  size_t capacity;        ///< exponent + 1 of how many total slots at `base`?
} dict_impl_t;

/// get the capacity (in slots) of a dictionary
static inline size_t dict_capacity(const dict_impl_t dict) {
  return (size_t)1 << dict.capacity >> 1;
}

/// atomically read a slot from a hash table
static inline dword_t slot_load(atomic_dword_t *slotptr) {
  return dword_atomic_load(slotptr);
}

/// atomically compare-and-swap into a hash table slot
static inline bool slot_cas(atomic_dword_t *slotptr, dword_t *expected,
                            dword_t desired) {
  return dword_atomic_cas(slotptr, expected, desired);
}

enum {
  MIGRATED = (uintptr_t)2, ///< mask for migration bit (see above)
  DELETED = (uintptr_t)1,  ///< mask for deletion bit (see above)
};

/// deserialise a slot back into its originating shared pointer
static inline sp_t slot_decode(dword_t slot) {
  sp_t decoded;
  assert(sizeof(slot) >= sizeof(decoded));
  memcpy(&decoded, &slot, sizeof(decoded));
  return decoded;
}

/// serialise a shared pointer into a slot
static inline dword_t slot_encode(sp_t ptr) {
  dword_t encoded = 0;
  assert(sizeof(encoded) >= sizeof(ptr));
  memcpy(&encoded, &ptr, sizeof(ptr));
  return encoded;
}

/// is this dictionary slot unoccupied?
static inline bool slot_is_free(dword_t slot) {
  const sp_t decoded = slot_decode(slot);
  return ((uintptr_t)decoded.ptr & ~MIGRATED) == 0;
}

/// does this dictionary slot contain an item that was deleted?
static inline bool slot_is_deleted(dword_t slot) {
  const sp_t decoded = slot_decode(slot);
  return ((uintptr_t)decoded.ptr & DELETED) != 0;
}

/// derive the equivalent deleted representation of a slot
static inline dword_t slot_deleted(dword_t slot) {
  assert(!slot_is_deleted(slot));
  sp_t decoded = slot_decode(slot);
  decoded.ptr = (void *)((uintptr_t)decoded.ptr | DELETED);
  return slot_encode(decoded);
}

/// has this slot been migrated to a new dictionary?
static inline bool slot_is_moved(dword_t slot) {
  const sp_t decoded = slot_decode(slot);
  return ((uintptr_t)decoded.ptr & MIGRATED) != 0;
}

/// derive the equivalent moved representation of a slot
static inline dword_t slot_moved(dword_t slot) {
  assert(!slot_is_moved(slot));
  sp_t p = slot_decode(slot);
  p.ptr = (void *)((uintptr_t)p.ptr | MIGRATED);
  return slot_encode(p);
}

/// convert a dictionary slot to its originating key pointer
static inline void *slot_to_ptr(dword_t slot) {
  sp_t decoded = slot_decode(slot);
  return (void *)((uintptr_t)decoded.ptr & ~(MIGRATED | DELETED));
}
