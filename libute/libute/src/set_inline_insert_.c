/// @file
/// @brief Implementation of set insertion, for inline set
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

int set_inline_insert_(set_t_ *set, void *item, bool *exists, set_sig_t_ sig) {
  assert(set != NULL);
  assert(item != NULL || sig.size == 0);
  assert(sig.count <= sizeof(set->raw) * CHAR_BIT);

  // materialise the value to insert
  uintptr_t value = 0;
  if (sig.size > 0)
    memcpy(&value, item, sig.size);

  // insert it
  const size_t word_offset = value / WORD_SIZE;
  assert(word_offset < sizeof(set->raw) / sizeof(set->raw[0]));
  const size_t bit_offset = value % WORD_SIZE;
  const uintptr_t old =
      word_or(&set->raw[word_offset], (uintptr_t)1 << bit_offset);

  if (exists != NULL)
    *exists = (old & ((uintptr_t)1 << bit_offset)) != 0;

  return 0;
}
