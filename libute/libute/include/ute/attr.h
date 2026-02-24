/// @file
/// @brief Portable use of attributes
///
/// All content in this file is in the public domain. Use it any way you wish.

#pragma once

/// mark a switch case as intentionally missing a `break` statement
#if defined(__cplusplus) && defined(__has_cpp_attribute) &&                    \
    __has_cpp_attribute(fallthrough)
#define FALLTHROUGH [[fallthrough]]
#elif !defined(__cplusplus) && defined(__has_c_attribute) &&                   \
    __has_c_attribute(fallthrough)
#define FALLTHROUGH [[fallthrough]]
#elif defined(__GNUC__)
#define FALLTHROUGH __attribute__((fallthrough))
#else
#define FALLTHROUGH /* nothing */
#endif

/// mark a function return value as should-not-be-ignored
#if defined(__cplusplus) && defined(__has_cpp_attribute) &&                    \
    __has_cpp_attribute(nodiscard)
#define NODISCARD [[nodiscard]]
#elif !defined(__cplusplus) && defined(__has_c_attribute) &&                   \
    __has_c_attribute(nodiscard)
#define NODISCARD [[nodiscard]]
#elif defined(__GNUC__)
#define NODISCARD __attribute__((warn_unused_result))
#else
#define NODISCARD /* nothing */
#endif

/// mark a function as never returning to its caller
#if defined(__cplusplus) && defined(__has_cpp_attribute) &&                    \
    __has_cpp_attribute(noreturn)
#define NORETURN [[noreturn]]
#elif !defined(__cplusplus) && defined(__has_c_attribute) &&                   \
    __has_c_attribute(noreturn)
#define NORETURN [[noreturn]]
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ == 201112L
#define NORETURN _Noreturn
#elif defined(__GNUC__)
#define NORETURN __attribute__((noreturn))
#else
#define NORETURN /* nothing */
#endif

/// mark a variable or function return value as being intentionally not used
#if defined(__cplusplus) && defined(__has_cpp_attribute) &&                    \
    __has_cpp_attribute(maybe_unused)
#define UNUSED [[maybe_unused]]
#elif !defined(__cplusplus) && defined(__has_c_attribute) &&                   \
    __has_c_attribute(maybe_unused)
#define UNUSED [[maybe_unused]]
#elif defined(__GNUC__)
#define UNUSED __attribute__((unused))
#else
#define UNUSED /* nothing */
#endif
