/// @file
/// @brief Dispatch for set contains
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "set.h"
#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>
#include <ute/set.h>

bool set_contains_(set_t_ *set, const void *item, set_sig_t_ sig) {

  if (sig.size < sizeof(uintptr_t) && sig.alignment <= alignof(uintptr_t))
    return set_unboxed_contains(set, item, sig);

  return set_boxed_contains(set, item, sig);
}
