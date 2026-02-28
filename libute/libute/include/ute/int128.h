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

#define INT128_MIN ((int128_t)((uint128_t)1 << 127))
#define INT128_MAX                                                             \
  ((int128_t)(UINT64_C(0xffffffffffffffff) |                                   \
              ((uint128_t)UINT64_C(0x7fffffffffffffff) << 64)))
#define UINT128_MAX                                                            \
  (UINT64_C(0xffffffffffffffff) |                                              \
   ((uint128_t)UINT64_C(0xffffffffffffffff) << 64))

///////////////////////////////////////////////////////////////////////////////
// atomics                                                                   //
///////////////////////////////////////////////////////////////////////////////

/// atomically read a 128-bit signed integer from memory
///
/// This function is lock-free. It performs the read with (at least) acquire
/// semantics.
///
/// Semantically this atomically performs:
///
///   return *src;
///
/// @param src Address to read from
/// @return Loaded value
int128_t int128_atomic_load(int128_t *src);

/// atomically write a 128-bit signed integer to memory
///
/// This function is lock-free. It performs the write with (at least) release
/// semantics.
///
/// Semantically this atomically performs:
///
///   *dst = src;
///
/// @param dst Location to write to
/// @param src Value to write
void int128_atomic_store(int128_t *dst, int128_t src);

/// atomically compare-and-swap a 128-bit signed integer
///
/// This function is lock-free. It performs a strong (no spurious failures)
/// compare-and-swap with (at least) acquire-release semantics.
///
/// Semantically this atomically performs:
///
///   bool result = *dst == *expected;
///   *expected = *dst;
///   if (result) {
///     *dst = desired;
///   }
///   return result;
///
/// @param dst Address to swap with
/// @param expected [in,out] Expected old value; actual old value on return
/// @param desired Value to try to write
/// @return True if the swap succeeded
bool int128_atomic_cas(int128_t *dst, int128_t *expected, int128_t desired);

/// atomically compare-and-swap a 128-bit signed integer
///
/// This function has the same semantics as `int128_atomic_cas_n` but takes
/// `expected` by value for callers who do not need the read old value.
///
/// @param dst Address to swap with
/// @param expected Expected old value
/// @param desired Value to try to write
/// @return True if the swap succeeded
bool int128_atomic_cas_n(int128_t *dst, int128_t expected, int128_t desired);

/// atomically read and write a 128-bit signed integer
///
/// This function is lock-free. It performs the exchange with (at least)
/// acquire-release semantics.
///
/// Semantically this atomically performs:
///
///   int128_t old = *dst;
///   *dst = src;
///   return old;
///
/// @param dst Location to read and write
/// @param src Value to write to `dst`
/// @return Original value read from `dst`
int128_t int128_atomic_xchg(int128_t *dst, int128_t src);

/// atomically read a 128-bit unsigned integer from memory
///
/// This function is lock-free. It performs the read with (at least) acquire
/// semantics.
///
/// Semantically this atomically performs:
///
///   return *src;
///
/// @param src Address to read from
/// @return Loaded value
uint128_t uint128_atomic_load(uint128_t *src);

/// atomically write a 128-bit unsigned integer to memory
///
/// This function is lock-free. It performs the write with (at least) release
/// semantics.
///
/// Semantically this atomically performs:
///
///   *dst = src;
///
/// @param dst Location to write to
/// @param src Value to write
void uint128_atomic_store(uint128_t *dst, uint128_t src);

/// atomically compare-and-swap a 128-bit unsigned integer
///
/// This function is lock-free. It performs a strong (no spurious failures)
/// compare-and-swap with (at least) acquire-release semantics.
///
/// Semantically this atomically performs:
///
///   bool result = *dst == *expected;
///   *expected = *dst;
///   if (result) {
///     *dst = desired;
///   }
///   return result;
///
/// @param dst Address to swap with
/// @param expected [in,out] Expected old value; actual old value on return
/// @param desired Value to try to write
/// @return True if the swap succeeded
bool uint128_atomic_cas(uint128_t *dst, uint128_t *expected, uint128_t desired);

/// atomically compare-and-swap a 128-bit unsigned integer
///
/// This function has the same semantics as `uint128_atomic_cas_n` but takes
/// `expected` by value for callers who do not need the read old value.
///
/// @param dst Address to swap with
/// @param expected Expected old value
/// @param desired Value to try to write
/// @return True if the swap succeeded
bool uint128_atomic_cas_n(uint128_t *dst, uint128_t expected, uint128_t desired);

/// atomically read and write a 128-bit unsigned integer
///
/// This function is lock-free. It performs the exchange with (at least)
/// acquire-release semantics.
///
/// Semantically this atomically performs:
///
///   int128_t old = *dst;
///   *dst = src;
///   return old;
///
/// @param dst Location to read and write
/// @param src Value to write to `dst`
/// @return Original value read from `dst`
uint128_t uint128_atomic_xchg(uint128_t *dst, uint128_t src);

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
