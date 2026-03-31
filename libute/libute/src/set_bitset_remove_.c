/// @file
/// @brief Implementation of set removal for bitset-backed set
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "set_bitset.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <ute/asp.h>
#include <ute/set.h>

bool set_bitset_remove_(set_t_ *set, const void *item, set_sig_t_ sig) {
  assert(set != NULL);
  assert(item != NULL || sig.size == 0);
  assert(sig.size <= 2);
  assert(sig.dtor == NULL);

  sp_t sp = sp_acq(&set->root);

  // if the bitset is not yet allocated, the set is empty
  if (sp.ptr == NULL)
    return false;

  // materialise the value to remove
  uintptr_t value = 0;
  if (sig.size > 0)
    memcpy(&value, item, sig.size);

  // remove it
  const size_t word_offset = value / WORD_SIZE;
  const size_t bit_offset = value % WORD_SIZE;
  _Atomic uintptr_t *const s = sp.ptr;
  const uintptr_t mask = (uintptr_t)1 << bit_offset;
  const uintptr_t previous = slot_and(&s[word_offset], ~mask);

  sp_rel(sp);
  return (previous & mask) != 0;
}
