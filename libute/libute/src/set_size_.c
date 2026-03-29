/// @file
/// @brief Dispatch for set size
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "set.h"
#include <stddef.h>
#include <ute/set.h>

size_t set_size_(set_t_ *set) { return set_boxed_size(set); }
