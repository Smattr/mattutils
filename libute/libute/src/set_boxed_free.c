/// @file
/// @brief Implementation of set destruction, for boxed set
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "set.h"
#include <assert.h>
#include <stddef.h>
#include <ute/asp.h>
#include <ute/set.h>

void set_boxed_free(set_t_ *set) {
  assert(set != NULL);

  // overwriting the root with a null pointer is enough to free the set
  sp_t null = sp_new(0, NULL, NULL);
  sp_store(&set->root, null);
}
