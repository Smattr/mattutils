/// @file
/// @brief Compatibility abstraction for `typeof` operator
///
/// All content in this file is in the public domain. Use it any way you wish.

#pragma once

#ifdef __STDC_VERSION__
#if __STDC_VERSION__ == 202311L
#define TYPEOF typeof
#endif
#endif
#if !defined(TYPEOF) && defined(__GNUC__) // Clang, GCC
#define TYPEOF __typeof__
#endif
#if !defined(TYPEOF) && defined(_MSC_VER) // MSVC
#define TYPEOF __typeof__
#endif
#ifndef TYPEOF
#error "no support for typeof"
#endif
