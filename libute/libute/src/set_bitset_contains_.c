/// @file
/// @brief Implementation of set existence check, for bitset-backed set
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "set_bitset.h"
#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <ute/asp.h>
#include <ute/set.h>

bool set_bitset_contains_(set_t_ *set, const void *item, set_sig_t_ sig) {
  assert(set != NULL);
  assert(item != NULL || sig.size == 0);
  assert(sig.size <= 2);
  assert(sig.dtor == NULL);

  sp_t sp = sp_acq(&set->root);

  // if the bitset is not yet allocated, the set is empty
  if (sp.ptr == NULL)
    return false;

  // materialise the value to find
  uintptr_t value = 0;
  if (sig.size > 0)
    memcpy(&value, item, sig.size);

  // load its containing word
  const size_t word_offset = value / WORD_SIZE;
  const size_t bit_offset = value % WORD_SIZE;
  atomic_uintptr_t *const s = sp.ptr;
  const uintptr_t word = slot_load(&s[word_offset]);

  sp_rel(sp);

  // is it present?
  return (word & ((uintptr_t)1 << bit_offset)) != 0;
}
