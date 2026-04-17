/// @file
/// @brief Inline set internals
///
/// The inline set implementation stores the presence of its elements as a
/// bitset _within_ the implementation struct itself.
///
/// All content in this file is in the public domain. Use it any way you wish.

#pragma once

#include <limits.h>
#include <stdatomic.h>
#include <stdint.h>

enum { WORD_SIZE = sizeof(uintptr_t) * CHAR_BIT };

static inline uintptr_t word_load(const _Atomic uintptr_t *word) {
  return atomic_load_explicit(word, memory_order_acquire);
}

static inline void word_store(_Atomic uintptr_t *dst, uintptr_t src) {
  atomic_store_explicit(dst, src, memory_order_release);
}

static inline uintptr_t word_and(_Atomic uintptr_t *word, uintptr_t src) {
  return atomic_fetch_and_explicit(word, src, memory_order_acq_rel);
}

static inline uintptr_t word_or(_Atomic uintptr_t *word, uintptr_t src) {
  return atomic_fetch_or_explicit(word, src, memory_order_acq_rel);
}
