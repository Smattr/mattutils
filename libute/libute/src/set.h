/// @file
/// @brief Set implementation alternatives
///
/// All content in this file is in the public domain. Use it any way you wish.

#pragma once

#include "attr.h"
#include <stdbool.h>
#include <stddef.h>
#include <ute/set.h>

////////////////////////////////////////////////////////////////////////////////
// implementations for boxed set
////////////////////////////////////////////////////////////////////////////////

PRIVATE int set_boxed_insert(set_t_ *set, const void *item, set_sig_t_ sig,
                             void (*user_dtor)(void *));
PRIVATE bool set_boxed_remove(set_t_ *set, const void *item, set_sig_t_ sig);
PRIVATE bool set_boxed_contains(set_t_ *set, const void *item, set_sig_t_ sig);
PRIVATE size_t set_boxed_size(set_t_ *set);
PRIVATE void set_boxed_free(set_t_ *set);
