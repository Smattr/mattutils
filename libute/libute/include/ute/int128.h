/// @file
/// @brief 128-bit scalar integer type interface
///
/// This is only usable on platforms that have native 128-bit integers (e.g.
/// x86-64).
///
/// All content in this file is in the public domain. Use it any way you wish.

#pragma once

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifndef __SIZEOF_INT128__
#error "128-bit integer type unavailable"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef __int128 int128_t;
typedef unsigned __int128 uint128_t;

#define INT128_MIN                                                             \
  ((int128_t)(UINT64_C(0xffffffffffffffff) |                                   \
              ((uint128_t)UINT64_C(0xffffffffffffffff) << 64)))
#define INT128_MAX                                                             \
  ((int128_t)(UINT64_C(0xffffffffffffffff) |                                   \
              ((uint128_t)UINT64_C(0x7fffffffffffffff) << 64)))
#define UINT128_MAX                                                            \
  (UINT64_C(0xffffffffffffffff) |                                              \
   ((uint128_t)UINT64_C(0xffffffffffffffff) << 64))

///////////////////////////////////////////////////////////////////////////////
// I/O                                                                       //
///////////////////////////////////////////////////////////////////////////////

/// print a 128-bit signed integer to an output stream
///
/// @param v Value to print
/// @param stream Stream to print to
/// @return A non-negative number on success or `EOF` on error
int int128_put(int128_t v, FILE *stream);

/// print a 128-bit unsigned integer to an output stream
///
/// @param v Value to print
/// @param stream Stream to print to
/// @return A non-negative number on success or `EOF` on error
int uint128_put(uint128_t v, FILE *stream);

#ifdef __cplusplus
}
#endif
