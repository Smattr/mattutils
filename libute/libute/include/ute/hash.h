/// @file
/// @brief Hashing of data
///
/// All content in this file is in the public domain. Use it any way you wish.

#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/// derive a hash of data, suitable for associative container addressing
///
/// Callers should not rely on the exact algorithm used.
///
/// @param data Data to hash
/// @param size Number of bytes at `data`
/// @return A hash digest of the data
size_t hash(const void *data, size_t size);

#ifdef __cplusplus
}
#endif
