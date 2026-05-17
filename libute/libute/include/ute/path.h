/// @file
/// @brief Functions for dealing with file system paths
///
/// All content in this file is in the public domain. Use it any way you wish.

#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// is this an absolute (as opposed to relative) path?
bool path_is_absolute(const char *path);

/// is this a relative (as opposed to absolute) path?
bool path_is_relative(const char *path);

#ifdef __cplusplus
}
#endif
