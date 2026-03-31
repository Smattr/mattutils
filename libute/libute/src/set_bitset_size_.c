/// @file
/// @brief Implementation of set size, for bitset-backed set
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "set_bitset.h"
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <ute/asp.h>
#include <ute/set.h>

size_t set_bitset_size_(set_t_ *set, set_sig_t_ sig) {
  assert(set != NULL);
  assert(sig.size <= 2);
  assert(sig.dtor == NULL);

  sp_t sp = sp_acq(&set->root);

  // if the bitset has not yet been allocated, the set is empty
  if (sp.ptr == NULL)
    return 0;

  // how wide is the bitset?
  const size_t bits = (size_t)1 << (sig.size * CHAR_BIT);
  const size_t words = bits / WORD_SIZE + (bits % WORD_SIZE == 0 ? 0 : 1);

  // count set bits
  const _Atomic uintptr_t *const s = sp.ptr;
  size_t size = 0;
  for (size_t i = 0; i < words; ++i) {
    const uintptr_t slot = slot_load(&s[i]);
    size += __builtin_popcountll(slot);
  }

  sp_rel(sp);
  return size;
}
