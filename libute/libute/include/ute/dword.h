/// @file
/// @brief Atomics on 2-pointer-wide scalars
///
/// How widely available are double-width atomics? It seems, almost everywhere.
///   https://timur.audio/dwcas-in-c
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

/// create a new dword
///
/// @return A zeroed dword
dword_t dword_zero(void);

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
///   if (result) {
///     *dst = desired;
///   } else {
///     *expected = *dst;
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
///   dword_t old = *dst;
///   *dst = src;
///   return old;
///
/// @param dst Location to read and write
/// @param src Value to write to `dst`
/// @return Original value read from `dst`
dword_t dword_atomic_xchg(atomic_dword_t *dst, dword_t src);

/// atomically read the lower half of a dword from memory
///
/// This function is lock-free. It performs the read with (at least) acquire
/// semantics.
///
/// Semantically this atomically performs:
///
///   return ((uintptr_t *)src)[0];
///
/// @param src Address to read from
/// @return Loaded value
uintptr_t dword_atomic_load_lo(atomic_dword_t *src);

/// atomically write to the lower half of a dword in memory
///
/// This function is lock-free. It performs the write with (at least) release
/// semantics.
///
/// Semantically this atomically performs:
///
///   ((uintptr_t *)dst)[0] = src;
///
/// @param dst Location to write to
/// @param src Value to write
void dword_atomic_store_lo(atomic_dword_t *dst, uintptr_t src);

/// atomically compare-and-swap the lower half of a dword
///
/// This function is lock-free. It performs a strong (no spurious failures)
/// compare-and-swap with (at least) acquire-release semantics.
///
/// Semantically this atomically performs:
///
///   uintptr_t *d = (uintptr_t *)dst;
///   bool result = d[0] == *expected;
///   if (result) {
///     d[0] = desired;
///   } else {
///     *expected = d[0];
///   }
///   return result;
///
/// @param dst Address to swap with
/// @param expected [in,out] Expected old value; actual old value on return
/// @param desired Value to try to write
/// @return True if the swap succeeded
bool dword_atomic_cas_lo(atomic_dword_t *dst, uintptr_t *expected,
                         uintptr_t desired);

/// atomically compare-and-swap the lower half of a dword
///
/// This function has the same semantics as `dword_atomic_cas_lo` but takes
/// `expected` by value for callers who do not need the read old value.
///
/// @param dst Address to swap with
/// @param expected Expected old value
/// @param desired Value to try to write
/// @return True if the swap succeeded
bool dword_atomic_cas_n_lo(atomic_dword_t *dst, uintptr_t expected,
                           uintptr_t desired);

/// atomically read and write the lower half of a dword
///
/// This function is lock-free. It performs the exchange with (at least)
/// acquire-release semantics.
///
/// Semantically this atomically performs:
///
///   uintptr_t *d = (uintptr_t *)dst;
///   uintptr_t old = d[0];
///   d[0] = src;
///   return old;
///
/// @param dst Location to read and write
/// @param src Value to write to `dst`
/// @return Original value read from `dst`
uintptr_t dword_atomic_xchg_lo(atomic_dword_t *dst, uintptr_t src);

/// atomically read the upper half of a dword from memory
///
/// This function is lock-free. It performs the read with (at least) acquire
/// semantics.
///
/// Semantically this atomically performs:
///
///   return ((uintptr_t *)src)[1];
///
/// @param src Address to read from
/// @return Loaded value
uintptr_t dword_atomic_load_hi(atomic_dword_t *src);

/// atomically write to the upper half of a dword in memory
///
/// This function is lock-free. It performs the write with (at least) release
/// semantics.
///
/// Semantically this atomically performs:
///
///   ((uintptr_t *)dst)[1] = src;
///
/// @param dst Location to write to
/// @param src Value to write
void dword_atomic_store_hi(atomic_dword_t *dst, uintptr_t src);

/// atomically compare-and-swap the upper half of a dword
///
/// This function is lock-free. It performs a strong (no spurious failures)
/// compare-and-swap with (at least) acquire-release semantics.
///
/// Semantically this atomically performs:
///
///   uintptr_t *d = (uintptr_t *)dst;
///   bool result = d[1] == *expected;
///   if (result) {
///     d[1] = desired;
///   } else {
///     *expected = d[1];
///   }
///   return result;
///
/// @param dst Address to swap with
/// @param expected [in,out] Expected old value; actual old value on return
/// @param desired Value to try to write
/// @return True if the swap succeeded
bool dword_atomic_cas_hi(atomic_dword_t *dst, uintptr_t *expected,
                         uintptr_t desired);

/// atomically compare-and-swap the upper half of a dword
///
/// This function has the same semantics as `dword_atomic_cas_hi` but takes
/// `expected` by value for callers who do not need the read old value.
///
/// @param dst Address to swap with
/// @param expected Expected old value
/// @param desired Value to try to write
/// @return True if the swap succeeded
bool dword_atomic_cas_n_hi(atomic_dword_t *dst, uintptr_t expected,
                           uintptr_t desired);

/// atomically read and write the upper half of a dword
///
/// This function is lock-free. It performs the exchange with (at least)
/// acquire-release semantics.
///
/// Semantically this atomically performs:
///
///   uintptr_t *d = (uintptr_t *)dst;
///   uintptr_t old = d[1];
///   d[1] = src;
///   return old;
///
/// @param dst Location to read and write
/// @param src Value to write to `dst`
/// @return Original value read from `dst`
uintptr_t dword_atomic_xchg_hi(atomic_dword_t *dst, uintptr_t src);

#ifdef __cplusplus
}
#endif
