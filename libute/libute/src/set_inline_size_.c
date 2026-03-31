/// @file
/// @brief Implementation of set size, for inline set
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "set_inline.h"
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <ute/set.h>

size_t set_inline_size_(set_t_ *set, set_sig_t_ sig) {
  assert(set != NULL);
  assert(sig.count <= sizeof(set->raw) * CHAR_BIT);
  (void)sig;

  // count the number of set bits in the raw representation
  size_t size = 0;
  for (size_t i = 0; i < sizeof(set->raw) / sizeof(set->raw[0]); ++i) {
    const uintptr_t word = word_load(&set->raw[i]);
    size += __builtin_popcountll(word);
  }

  return size;
}
