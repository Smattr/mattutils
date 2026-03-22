/// @file
/// @brief Portable use of attributes
///
/// All content in this file is in the public domain. Use it any way you wish.

#pragma once

#include <stddef.h>
#include <stdlib.h>

/// mark a switch case as intentionally missing a `break` statement
#if defined(__cplusplus) && defined(__has_cpp_attribute)
#if __has_cpp_attribute(fallthrough)
#define FALLTHROUGH [[fallthrough]]
#endif
#endif
#if !defined(FALLTHROUGH) && !defined(__cplusplus) && defined(__has_c_attribute)
#if __has_c_attribute(fallthrough)
#define FALLTHROUGH [[fallthrough]]
#endif
#endif
#if !defined(FALLTHROUGH) && defined(__GNUC__)
#define FALLTHROUGH __attribute__((fallthrough))
#endif
#ifndef FALLTHROUGH
#define FALLTHROUGH /* nothing */
#endif

/// mark a function return value as should-not-be-ignored
#if defined(__cplusplus) && defined(__has_cpp_attribute)
#if __has_cpp_attribute(nodiscard)
#define NODISCARD [[nodiscard]]
#endif
#endif
#if !defined(NODISCARD) && !defined(__cplusplus) && defined(__has_c_attribute)
#if __has_c_attribute(nodiscard)
#define NODISCARD [[nodiscard]]
#endif
#endif
#if !defined(NODISCARD) && defined(__GNUC__)
#define NODISCARD __attribute__((warn_unused_result))
#endif
#ifndef NODISCARD
#define NODISCARD /* nothing */
#endif

/// mark a function as never returning to its caller
#if defined(__cplusplus) && defined(__has_cpp_attribute)
#if __has_cpp_attribute(noreturn)
#define NORETURN [[noreturn]]
#endif
#endif
#if !defined(NORETURN) && !defined(__cplusplus) && defined(__has_c_attribute)
#if __has_c_attribute(noreturn)
#define NORETURN [[noreturn]]
#endif
#endif
#if !defined(NORETURN) && defined(__STDC_VERSION__)
#if __STDC_VERSION__ == 201112L
#define NORETURN _Noreturn
#endif
#endif
#if !defined(NORETURN) && defined(__GNUC__)
#define NORETURN __attribute__((noreturn))
#endif
#ifndef NORETURN
#define NORETURN /* nothing */
#endif

#if !defined(UNREACHABLE) && defined(__cplusplus)
#if __cplusplus == 202302L
#include <utility>
#define UNREACHABLE()                                                          \
  do {                                                                         \
    assert(0 && !"unreachable code reached");                                  \
    std::unreachable();                                                        \
  } while (0)
#endif
#endif
#if !defined(UNREACHABLE) && defined(__STDC_VERSION__)
#if __STDC_VERSION__ == 202311L
#define UNREACHABLE()                                                          \
  do {                                                                         \
    assert(0 && !"unreachable code reached");                                  \
    unreachable();                                                             \
  } while (0)
#endif
#endif
#if !defined(UNREACHABLE) && defined(__GNUC__)
#define UNREACHABLE()                                                          \
  do {                                                                         \
    assert(0 && !"unreachable code reached");                                  \
    __builtin_unreachable();                                                   \
  } while (0)
#endif
#ifndef UNREACHABLE
#define UNREACHABLE()                                                          \
  do {                                                                         \
    assert(0 && !"unreachable code reached");                                  \
    abort();                                                                   \
  } while (0)
#endif

/// mark a variable or function return value as being intentionally not used
#if defined(__cplusplus) && defined(__has_cpp_attribute)
#if __has_cpp_attribute(maybe_unused)
#define UNUSED [[maybe_unused]]
#endif
#endif
#if !defined(UNUSED) && !defined(__cplusplus) && defined(__has_c_attribute)
#if __has_c_attribute(maybe_unused)
#define UNUSED [[maybe_unused]]
#endif
#endif
#if !defined(UNUSED) && defined(__GNUC__)
#define UNUSED __attribute__((unused))
#endif
#ifndef UNUSED
#define UNUSED /* nothing */
#endif
