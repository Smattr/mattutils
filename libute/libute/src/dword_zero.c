/// @file
/// @brief Implementation of dword initialisation
///
/// All content in this file is in the public domain. Use it any way you wish.

#include <string.h>
#include <ute/dword.h>

dword_t dword_zero(void) {
  dword_t d;
  memset(&d, 0, sizeof(d));
  return d;
}
