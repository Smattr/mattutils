/// @file
/// @brief Implementation of set insertion, for bitset-backed set
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "set_bitset.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ute/asp.h>
#include <ute/attr.h>
#include <ute/set.h>

static void dtor(void *s, void *ignored UNUSED) { free(s); }

int set_bitset_insert_(set_t_ *set, void *item, set_sig_t_ sig) {
  assert(set != NULL);
  assert(item != NULL || sig.size == 0);
  assert(sig.size <= 2);
  assert(sig.dtor == NULL);

retry:;

  sp_t sp = sp_acq(&set->root);

  // do we need to allocate the bitset?
  if (sp.ptr == NULL) {
    const size_t bits = (size_t)1 << (sig.size * CHAR_BIT);
    const size_t words = bits / WORD_SIZE + (bits % WORD_SIZE == 0 ? 0 : 1);
    _Atomic uintptr_t *const s = calloc(words, sizeof(s[0]));
    if (s == NULL)
      return ENOMEM;

    sp_t new_sp = sp_new(s, dtor, NULL);
    if (new_sp.ptr == NULL) {
      dtor(s, NULL);
      sp_rel(sp);
      return ENOMEM;
    }

    if (!sp_cas(&set->root, sp, new_sp))
      sp_rel(new_sp);
    sp_rel(sp);
    goto retry;
  }

  // matrialise the value to insert
  uintptr_t value = 0;
  if (sig.size > 0)
    memcpy(&value, item, sig.size);

  // insert it
  const size_t word_offset = value / WORD_SIZE;
  const size_t bit_offset = value % WORD_SIZE;
  _Atomic uintptr_t *const s = sp.ptr;
  slot_or(&s[word_offset], (uintptr_t)1 << bit_offset);

  sp_rel(sp);
  return 0;
}
