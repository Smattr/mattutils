/// @file
/// @brief Equivalents of some of the ctype.h functions
///
/// The Glibc ctype.h character class functions are surprisingly slow because
/// they go through locale tables. This seems to be true even for ones like
/// `isdigit` that does not depend on locale. Additionally, the standardised
/// behaviour of these functions has a number of quirks:
///   1. Though they take `int` values, passing a value outside of `EOF` or the
///      range of `unsigned char` is UB. Some platforms, like Windows, have
///      taken the standard at its word and crash if passed out-of-range values.
///   2. Because their return type is `int`, many implementations take advantage
///      of this to return non-0 non-1 values for a “yes” answer. This often
///      leads to subtle bugs in calling code that is only anticipating boolean
///      returns.
///
/// This header implements alternatives that are both faster and safer.
///
/// All content in this file is in the public domain. Use it any way you wish.

#pragma once

#include <stdbool.h>
#include <string.h>

static inline bool is_ascii(int c) { return c >= 0 && c <= 127; }

static inline bool is_lower(int c) { return c >= 'a' && c <= 'z'; }

static inline bool is_upper(int c) { return c >= 'A' && c <= 'Z'; }

static inline bool is_alpha(int c) { return is_upper(c) || is_lower(c); }

static inline bool is_digit(int c) { return c >= '0' && c <= '9'; }

static inline bool is_alnum(int c) { return is_alpha(c) || is_digit(c); }

static inline bool is_blank(int c) { return c == ' ' || c == '\t'; }

static inline bool is_cntrl(int c) { return (c >= 0 && c <= 31) || c == 127; }

static inline bool is_space(int c) { return strchr("\t\n\v\f\r ", c) != NULL; }

static inline bool is_xdigit(int c) {
  if (c >= 'a' && c <= 'f')
    return true;
  if (c >= 'A' && c <= 'F')
    return true;
  return is_digit(c);
}
