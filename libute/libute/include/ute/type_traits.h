/// @file
/// @brief C equivalents to C++’s <type_traits>
///
/// All content in this file is in the public domain. Use it any way you wish.

#pragma once

/// is the given type an integral type?
#ifdef __SIZEOF_INT128__
#define is_integral(t)                                                         \
  _Generic(*(t *){0},                                                          \
      __int128: 1,                                                             \
      unsigned __int128: 1,                                                    \
      default: is_integral_bool_(t) || is_integral_(t))
#else
#define is_integral(t) (is_integral_bool_(t) || is_integral_(t))
#endif

#if defined(__STDC_VERSION__)
#if __STDC_VERSION__ >= 202311L
#define is_integral_bool_(t) _Generic(*(t *){0}, bool: 1, default: 0)
#endif
#endif
#ifndef is_integral_bool_
#define is_integral_bool_(t) _Generic(*(t *){0}, _Bool: 1, default: 0)
#endif

#define is_integral_(t)                                                        \
  _Generic(*(t *){0},                                                          \
      char: 1,                                                                 \
      signed char: 1,                                                          \
      unsigned char: 1,                                                        \
      short: 1,                                                                \
      unsigned short: 1,                                                       \
      int: 1,                                                                  \
      unsigned: 1,                                                             \
      long: 1,                                                                 \
      unsigned long: 1,                                                        \
      long long: 1,                                                            \
      unsigned long long: 1,                                                   \
      default: 0)

/// is the given integral type signed?
#define is_signed(t) ((t){-1} < (t){0})

/// is the given integral type unsigned?
#define is_unsigned(t) ((t){0} < (t){-1})
