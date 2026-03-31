/// @file
/// @brief Implementation of set destruction, for inline set
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "set_inline.h"
#include <assert.h>
#include <stddef.h>
#include <ute/set.h>

void set_inline_free_(set_t_ *set) {
  assert(set != NULL);

  // just clear the bitset
  for (size_t i = 0; i < sizeof(set->raw) / sizeof(set->raw[0]); ++i)
    word_store(&set->raw[i], 0);
}
