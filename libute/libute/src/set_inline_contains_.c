/// @file
/// @brief Implementation of set existence check, for inline set
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "set_inline.h"
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <ute/set.h>

bool set_inline_contains_(set_t_ *set, const void *item, set_sig_t_ sig) {
  assert(set != NULL);
  assert(item != NULL || sig.size == 0);
  assert(sig.count <= sizeof(set->raw) * CHAR_BIT);

  // materialise the value to find
  uintptr_t value = 0;
  if (sig.size > 0)
    memcpy(&value, item, sig.size);

  // load its containing word
  const size_t word_offset = value / WORD_SIZE;
  assert(word_offset < sizeof(set->raw) / sizeof(set->raw[0]));
  const size_t bit_offset = value % WORD_SIZE;
  const uintptr_t word = word_load(&set->raw[word_offset]);

  // is it present?
  return (word & ((uintptr_t)1 << bit_offset)) != 0;
}
