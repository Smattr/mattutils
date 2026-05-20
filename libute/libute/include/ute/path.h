/// @file
/// @brief Functions for dealing with file system paths
///
/// The functions below, with the exception of `path_getcwd`, deal with paths
/// purely as strings, not making any decisions based on file system contents.
/// This is a deliberate design to guide users away from
/// Time-Of-Check-to-Time-Of-Use (TOCTOU) problems. Any time you need to relate
/// a path to current file system contents, you should be operating on open file
/// descriptors (possibly opened with `O_PATH`) instead of path strings.
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
