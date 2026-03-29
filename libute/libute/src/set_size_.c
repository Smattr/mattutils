/// @file
/// @brief Dispatch for set size
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "set.h"
#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <ute/set.h>

size_t set_size_(set_t_ *set, set_sig_t_ sig) {

  if (sig.size < sizeof(uintptr_t) && sig.alignment <= alignof(uintptr_t))
    return set_unboxed_size(set);

  return set_boxed_size(set);
}
