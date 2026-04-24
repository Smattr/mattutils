/// @file
/// @brief Compatibility wrapper around `aligned_alloc`
///
/// All content in this file is in the public domain. Use it any way you wish.

#pragma once

#include <stdlib.h>

#ifdef _MSC_VER
#include <malloc.h>
#endif

/// allocate memory with a given alignment
///
/// This function has the same semantics as `aligned_alloc`, with the exception
/// that memory allocated through this function must be freed through
/// `aligned_free`.
///
/// @param alignment Requested alignment in bytes
/// @param size Requested number of bytes
/// @return Allocated memory on success or `NULL` on failure
static inline void *alloc_aligned(size_t alignment, size_t size) {
#ifdef _MSC_VER
  return _aligned_malloc(size, alignment);
#else
  return aligned_alloc(alignment, size);
#endif
}

/// deallocate memory allocated by `alloc_aligned`
///
/// Calling this on `NULL` is a no-op.
///
/// @param p Pointer to memory to free
static inline void free_aligned(void *p) {
#ifdef _MSC_VER
  _aligned_free(p);
#else
  free(p);
#endif
}
