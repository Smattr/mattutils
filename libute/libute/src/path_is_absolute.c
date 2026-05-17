/// @file
/// @brief Implementation of path absolute/relative discrimination
///
/// All content in this file is in the public domain. Use it any way you wish.

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <ute/path.h>

bool path_is_absolute(const char *path) {
  assert(path != NULL);

  const enum {
    MINGW,
    POSIX,
    WINDOWS
  } platform =
#ifdef __MINGW32__
      MINGW
#elif defined(_WIN32)
      WINDOWS
#else
      POSIX
#endif
      ;

  // MinGW appears to support both / and \ as separators but prefers /
  if (platform == MINGW && path[0] == '/')
    return true;
  if (platform == MINGW && path[0] == '\\')
    return true;

  // BSDs, Linux, macOS
  if (platform == POSIX && path[0] == '/')
    return true;

  if (platform == WINDOWS) {
    // e.g. C:\foo
    if (path[0] != '\0' && path[1] == ':') {
      if (path[2] == '/')
        return true;
      if (path[2] == '\\')
        return true;
    }

    // e.g. \\my\network\share
    if (path[0] == '/' || path[0] == '\\') {
      if (path[1] == '/' || path[1] == '\\')
        return true;
    }
  }

  return false;
}

bool path_is_relative(const char *path) {
  assert(path != NULL);

  return !path_is_absolute(path);
}
