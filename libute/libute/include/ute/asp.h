/// @file
/// @brief Atomic shared pointers
///
/// A shared pointer is a pointer multiple threads can access, with the last
/// thread to release its handle to the pointer triggering a destructor call. An
/// atomic shared pointer is the same thing with the additional feature that the
/// originating location of handles to a pointer can be overwritten/updated by
/// any thread without leaking memory or corrupting state.
///
/// All content in this file is in the public domain. Use it any way you wish.

#pragma once

#include <stdbool.h>
#include <ute/dword.h>

#ifdef __cplusplus
extern "C" {
#endif

/// control block for shared pointers
typedef struct sp_ctrl sp_ctrl_t;

/// a shared pointer
///
/// Analogous to C++’s `std::shared_ptr<…>`. These should only be constructed by
/// either:
///   1. Zero-initialization, e.g. `sp_t s = {0}`; or
///   2. `sp_new`; or
///   3. `sp_acq`.
/// Any constructed pointer should eventually be destructed with `sp_rel`. Pay
/// special attention to these semantics as the compiler cannot spot missing
/// releases.
typedef struct {
  void *ptr;       ///< the managed underlying pointer
  sp_ctrl_t *impl; ///< private state
} sp_t;

/// an atomic shared pointer
///
/// Analogous to C++’s `std::atomic<std::shared_ptr<…>>`. This representation is
/// intended to be opaque. Includers should only interact with values of this
/// type through the API below.
typedef atomic_dword_t asp_t;

/// create a new shared pointer
///
/// Creation of a shared pointer equivalent of the null pointer always succeeds
/// and returns `(sp_t){0}`.
///
/// @param value Raw pointer to encapsulate
/// @param dtor Optional destructor to be called when last `sp_t` is released
/// @param dtor_context Value to pass in `dtor` calls as second parameter
/// @return A shared pointer or `(sp_t){0}` on failure.
sp_t sp_new(void *value, void (*dtor)(void *, void *), void *dtor_context);

/// load a shared pointer
///
/// Any pointer loaded through this function should eventually be released
/// (`sp_rel`).
///
/// @param asp Pointer to load
/// @return Loaded shared pointer
sp_t sp_acq(asp_t *asp);

/// release a shared pointer
///
/// If calling this function releases the last live `sp_t` to the underlying
/// pointer, the destructor will be called and the metadata freed.
///
/// Calling this on a null shared pointer is a no-op.
///
/// @param sp Pointer to release
void sp_rel(sp_t sp);

/// atomically overwrite a shared pointer
///
/// This function consumes `src`, so the caller should not try to `sp_rel` it.
///
/// @param dst Pointer to overwrite
/// @param src Pointer value to store
void sp_store(asp_t *dst, sp_t src);

/// atomically compare-and-swap a shared pointer
///
/// `expected` is consumed (`sp_rel` is called on it) regardless of whether the
/// CAS succeeds or fails. It is assumed that any caller wanting to retry a
/// failed CAS will have to acquire a new (different) `expected` before
/// recalling this function. Note that this means, in contrast to many CAS APIs,
/// there is no way for the caller to directly learn the value that was read on
/// failure.
///
/// `desired` is consumed iff the CAS succeeds.
///
/// Note that by requiring an `sp_t` for `expected` instead of a raw pointer,
/// we implicitly rule out an ABA problem by the caller having to prove they
/// have an outstanding live pointer to the expected value. Without this, it
/// would be possible for someone else to deallocate the expected value and its
/// control block and then reallocate something new with coincidentally the same
/// addresses.
///
/// @param dst Pointer to overwrite on success
/// @param expected Expected previous value of `dst`
/// @param desired New value to set `dst` to
/// @return True if the CAS succeeded
bool sp_cas(asp_t *dst, sp_t expected, sp_t desired);

#ifdef __cplusplus
}
#endif
