/// @file
/// @brief Boxed set internals
///
/// The set implemented below stores its elements out-of-line in heap-allocated
/// memory.
///
/// All content in this file is in the public domain. Use it any way you wish.

#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <ute/asp.h>
#include <ute/dword.h>
#include <ute/set.h>

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
  /// The slots are shared pointers, made up of two words. The high bits of each
  /// slot’s `ptr` field are the actual raw pointer and the low bits indicate
  /// the state of the slot:
  ///
  ///                 ┌─ sizeof(uintptr_t) * CHAR_BIT - 1
  ///                 │                              1 0
  ///                 ▼                              ▼ ▼
  ///   base[i].ptr: ┌──────────────────────────────┬─┬─┐
  ///                └──────────────────────────────┴─┴─┘
  ///                       pointer to item          ▲ ▲
  ///                                                │ │
  ///                            has been migrated? ─┘ │
  ///                               has been deleted? ─┘
  atomic_dword_t *base;

  _Atomic size_t used;    ///< how many slots are non-empty?
  _Atomic size_t deleted; ///< how many slots contain deleted items?
  size_t capacity;        ///< exponent + 1 of how many total slots at `base`?
} set_impl_t;

/// get the capacity (in slots) of a set
static inline size_t set_capacity(const set_impl_t set) {
  return (size_t)1 << set.capacity >> 1;
}

/// atomically read a slot from a hash table
static inline dword_t slot_load(atomic_dword_t *slotptr) {
  return dword_atomic_load(slotptr);
}

/// atomically read the lower word of a slot from the hash table
static inline uintptr_t half_slot_load(atomic_dword_t *slotptr) {
  return dword_atomic_load_lo(slotptr);
}

/// atomically compare-and-swap into a hash table slot
static inline bool slot_cas(atomic_dword_t *slotptr, dword_t *expected,
                            dword_t desired) {
  return dword_atomic_cas(slotptr, expected, desired);
}

/// atomically compare-and-swap into the lower word of a hash table slot
static inline bool half_slot_cas(atomic_dword_t *slotptr, uintptr_t *expected,
                                 uintptr_t desired) {
  return dword_atomic_cas_lo(slotptr, expected, desired);
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

/// deserialise a half slot back into its originating shared pointer
static inline void *half_slot_decode(uintptr_t half_slot) {
  return (void *)half_slot;
}

/// serialise a shared pointer into a slot
static inline dword_t slot_encode(sp_t ptr) {
  dword_t encoded = 0;
  assert(sizeof(encoded) >= sizeof(ptr));
  memcpy(&encoded, &ptr, sizeof(ptr));
  return encoded;
}

/// is this set half slot unoccupied?
static inline bool half_slot_is_free(uintptr_t half_slot) {
  return (half_slot & ~MIGRATED) == 0;
}

/// is this set slot unoccupied?
static inline bool slot_is_free(dword_t slot) {
  const sp_t decoded = slot_decode(slot);
  return half_slot_is_free((uintptr_t)decoded.ptr);
}

/// does this set half slot contain an item that was deleted?
static inline bool half_slot_is_deleted(uintptr_t half_slot) {
  return (half_slot & DELETED) != 0;
}

/// does this set slot contain an item that was deleted?
static inline bool slot_is_deleted(dword_t slot) {
  const sp_t decoded = slot_decode(slot);
  return half_slot_is_deleted((uintptr_t)decoded.ptr);
}

/// derive the equivalent deleted representation of a half_slot
static inline uintptr_t half_slot_deleted(uintptr_t half_slot) {
  assert(!half_slot_is_deleted(half_slot));
  return half_slot | DELETED;
}

/// has this half slot been migrated to a new set?
static inline bool half_slot_is_moved(uintptr_t half_slot) {
  return (half_slot & MIGRATED) != 0;
}

/// has this slot been migrated to a new set?
static inline bool slot_is_moved(dword_t slot) {
  const sp_t decoded = slot_decode(slot);
  return half_slot_is_moved((uintptr_t)decoded.ptr);
}

/// derive the equivalent moved representation of a slot
static inline dword_t slot_moved(dword_t slot) {
  assert(!slot_is_moved(slot));
  sp_t p = slot_decode(slot);
  p.ptr = (void *)((uintptr_t)p.ptr | MIGRATED);
  return slot_encode(p);
}

/// convert a set half slot to its originating item pointer
static inline void *half_slot_to_ptr(uintptr_t half_slot) {
  return (void *)(half_slot & ~(MIGRATED | DELETED));
}

/// convert a set slot to its originating item pointer
static inline void *slot_to_ptr(dword_t slot) {
  const sp_t decoded = slot_decode(slot);
  return half_slot_to_ptr((uintptr_t)decoded.ptr);
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
