/// @file
/// @brief Dispatch for set free
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "set.h"
#include <ute/set.h>

void set_free_(set_t_ *set) { return set_boxed_free(set); }
