/// @file
/// @brief Compatibility wrapper around `aligned_alloc`
///
/// All content in this file is in the public domain. Use it any way you wish.

#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/// allocate memory with a given alignment
///
/// This macro has the same semantics as `aligned_alloc`, with the exception
/// that memory allocated through this function must be freed through
/// `ALIGNED_FREE`.
///
/// @param alignment Requested alignment in bytes
/// @param size Requested number of bytes
/// @return Allocated memory on success or `NULL` on failure
#define ALIGNED_ALLOC(alignment, size)                                         \
  aligned_alloc_((alignment), (size), __FILE__, __LINE__)

/// deallocate memory allocated by `alloc_aligned`
///
/// Calling this on `NULL` is a no-op.
///
/// @param p Pointer to memory to free
#define ALIGNED_FREE(ptr) aligned_free_((ptr), __FILE__, __LINE__)

/// allocate memory with a given alignment
///
/// This function should be considered private and not to be called directly.
///
/// @param alignment Requested alignment in bytes
/// @param size Requested number of bytes
/// @return Allocated memory on success or `NULL` on failure
void *aligned_alloc_(size_t alignment, size_t size, const char *filename,
                     int lineno);

/// deallocate memory allocated by `aligned_alloc_`
///
/// This function should be considered private and not to be called directly.
///
/// @param p Pointer to memory to free
void aligned_free_(void *ptr, const char *filename, int lineno);

#ifdef __cplusplus
}
#endif
