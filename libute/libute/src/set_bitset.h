/// @file
/// @brief Bitset-backed set internals
///
/// The bitset-backed implementation stores the presence of its elements as a
/// bitset array.
///
/// All content in this file is in the public domain. Use it any way you wish.

#pragma once

#include <limits.h>
#include <stdatomic.h>
#include <stdint.h>

enum { WORD_SIZE = sizeof(uintptr_t) * CHAR_BIT };

static inline uintptr_t slot_load(const atomic_uintptr_t *slot) {
  return atomic_load_explicit(slot, memory_order_acquire);
}

static inline uintptr_t slot_and(atomic_uintptr_t *slot, uintptr_t src) {
  return atomic_fetch_and_explicit(slot, src, memory_order_acq_rel);
}

static inline uintptr_t slot_or(atomic_uintptr_t *slot, uintptr_t src) {
  return atomic_fetch_or_explicit(slot, src, memory_order_acq_rel);
}
