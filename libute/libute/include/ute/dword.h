/// @file
/// @brief Atomics on 2-pointer-wide scalars
///
/// All content in this file is in the public domain. Use it any way you wish.

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifndef __SIZEOF_POINTER__
#error "sizeof pointer not available"
#endif

#if __SIZEOF_POINTER__ == 8
#include <ute/int128.h>

/// a 2-pointer-wide scalar type
typedef uint128_t dword_t;

/// a 2-pointer-wide scalar type that must be operated on atomically
typedef dword_t atomic_dword_t;

#elif __SIZEOF_POINTER__ == 4

/// a 2-pointer-wide scalar type
typedef uint64_t dword_t;

/// a 2-pointer-wide scalar type that must be operated on atomically
typedef _Atomic uint64_t atomic_dword_t;

#else
#error "unimplemented"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/// atomically read a dword from memory
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
dword_t dword_atomic_load(atomic_dword_t *src);

/// atomically write a dword to memory
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
void dword_atomic_store(atomic_dword_t *dst, dword_t src);

/// atomically compare-and-swap a dword
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
bool dword_atomic_cas(atomic_dword_t *dst, dword_t *expected, dword_t desired);

/// atomically compare-and-swap a dword
///
/// This function has the same semantics as `dword_atomic_cas_n` but takes
/// `expected` by value for callers who do not need the read old value.
///
/// @param dst Address to swap with
/// @param expected Expected old value
/// @param desired Value to try to write
/// @return True if the swap succeeded
bool dword_atomic_cas_n(atomic_dword_t *dst, dword_t expected, dword_t desired);

/// atomically read and write a dword
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
dword_t dword_atomic_xchg(atomic_dword_t *dst, dword_t src);

#ifdef __cplusplus
}
#endif
