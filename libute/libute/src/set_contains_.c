/// @file
/// @brief Dispatch for set contains
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "set.h"
#include <stdbool.h>
#include <ute/set.h>

bool set_contains_(set_t_ *set, const void *item, set_sig_t_ sig) {
  return set_boxed_contains(set, item, sig);
}
