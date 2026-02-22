/// @file
/// @brief Abstraction for teaching the compiler optimization hints
///
/// All content in this file is in the public domain. Use it any way you wish.

#pragma once

/// assume an expression is true
///
/// This is a primitive for teaching the compiler something it can then use as
/// an optimization hint.
#ifdef __clang__
#define ASSUME(expr) __builtin_assume(expr)
#elif defined(__GNUC__)
#define ASSUME(expr)                                                           \
  do {                                                                         \
    if (!(expr)) {                                                             \
      __builtin_unreachable();                                                 \
    }                                                                          \
  } while (0)
#elif defined(_MSC_VER)
#define ASSUME(expr) __assume(expr)
#else
#define ASSUME(expr) /* nothing */
#endif
