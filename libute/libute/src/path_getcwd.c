/// @file
/// @brief Implementation of current working directory retrieval
///
/// All content in this file is in the public domain. Use it any way you wish.

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <ute/path.h>

#ifdef _MSC_VER
#include <direct.h>
#endif

#ifndef _MSC_VER
#include <unistd.h>
#endif

int path_getcwd(char **wd) {

  if (wd == NULL)
    return EINVAL;

  *wd = NULL;

#ifdef _MSC_VER
  {
    size_t maxlen = 1;
    while (true) {
      char *const ret = _getcwd(NULL, maxlen);
      if (ret != NULL) {
        *wd = ret;
        break;
      }

      if (errno != ERANGE)
        return errno;
    }
    return 0;
  }
#else
  {
    // every platform other than Windows seems to support the extension of heap
    // allocating enough memory when passing NULL, 0
    char *const ret = getcwd(NULL, 0);
    if (ret == NULL)
      return errno;

    *wd = ret;
    return 0;
  }
#endif
}
