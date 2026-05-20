/// @file
/// @brief Functions for dealing with file system paths
///
/// All content in this file is in the public domain. Use it any way you wish.

#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// get the path to the current working directory
///
/// This function behaves similarly to `getcwd` and has many of the same
/// caveats. The value returned in `wd` should eventually be passed to `free`.
///
/// @param wd [out] Current working directory on success
/// @return 0 on success or an errno on failure
int path_getcwd(char **wd);

/// is this an absolute (as opposed to relative) path?
bool path_is_absolute(const char *path);

/// is this a relative (as opposed to absolute) path?
bool path_is_relative(const char *path);

#ifdef __cplusplus
}
#endif
