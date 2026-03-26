/// @file
/// @brief Abstraction for teaching the compiler optimization hints
///
/// All content in this file is in the public domain. Use it any way you wish.

#pragma once

/// assume an expression is true
///
/// This is a primitive for teaching the compiler something it can then use as
/// an optimization hint.
#if !defined(ASSUME) && defined(__cplusplus) && defined(__has_cpp_attribute)
#if __has_cpp_attribute(assume)
#define ASSUME(expr) [[assume(expr)]]
#endif
#endif
#if !defined(ASSUME) && defined(__clang__)
#define ASSUME(expr) __builtin_assume(expr)
#endif
#if !defined(ASSUME) && defined(__GNUC__)
#define ASSUME(expr)                                                           \
  do {                                                                         \
    if (!(expr)) {                                                             \
      __builtin_unreachable();                                                 \
    }                                                                          \
  } while (0)
#endif
#if !defined(ASSUME) && defined(_MSC_VER)
#define ASSUME(expr) __assume(expr)
#endif
#ifndef ASSUME
#define ASSUME(expr) /* nothing */
#endif
