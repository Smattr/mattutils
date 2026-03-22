/// @file
/// @brief Implementation of set size
///
/// All content in this file is in the public domain. Use it any way you wish.

#include <assert.h>
#include <stddef.h>
#include <ute/set.h>

size_t set_size_(const set_t_ *set) {
  assert(set != NULL);
  assert(set->used >= set->deleted);
  return set->used - set->deleted;
}
