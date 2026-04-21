/// @file
/// @brief Implementation of dictionary destruction
///
/// All content in this file is in the public domain. Use it any way you wish.

#include <assert.h>
#include <stddef.h>
#include <ute/asp.h>
#include <ute/dict.h>

void dict_free_(dict_t_ *dict) {
  assert(dict != NULL);

  // overwriting the root with a null pointer is enough to free the dictionary
  sp_t null = sp_new(0, NULL, NULL);
  sp_store(&dict->root, null);
}
