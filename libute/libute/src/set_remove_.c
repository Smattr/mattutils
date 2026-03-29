/// @file
/// @brief Dispatch for set removal
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "set.h"
#include <stdbool.h>
#include <ute/set.h>

bool set_remove_(set_t_ *set, const void *item, set_sig_t_ sig) {
  return set_boxed_remove(set, item, sig);
}
