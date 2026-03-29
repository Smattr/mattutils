/// @file
/// @brief Dispatch for set insertion
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "set.h"
#include <ute/set.h>

int set_insert_(set_t_ *set, const void *item, set_sig_t_ sig,
                void (*user_dtor)(void *)) {
  return set_boxed_insert(set, item, sig, user_dtor);
}
