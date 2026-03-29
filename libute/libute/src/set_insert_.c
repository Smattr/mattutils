/// @file
/// @brief Dispatch for set insertion
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "set.h"
#include <stdalign.h>
#include <stdint.h>
#include <ute/set.h>

int set_insert_(set_t_ *set, const void *item, set_sig_t_ sig,
                void (*user_dtor)(void *)) {

  if (sig.size < sizeof(uintptr_t) && sig.alignment <= alignof(uintptr_t))
    return set_unboxed_insert(set, item, sig, user_dtor);

  return set_boxed_insert(set, item, sig, user_dtor);
}
