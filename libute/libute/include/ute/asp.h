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
/// @return A shared pointer or `(sp_t){0}` on failure.
sp_t sp_new(void *value, void (*dtor)(void *));

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
/// @param sp Pointer to release
void sp_rel(sp_t sp);

/// atomically overwrite a shared pointer
///
/// This function consumes `src`, so the caller should not try to `sp_rel` it.
///
/// @param dst Pointer to overwrite
/// @param src Pointer value to store
void sp_store(asp_t *dst, sp_t src);

#ifdef __cplusplus
}
#endif
