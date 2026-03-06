/// @file
/// @brief Extending printing utilities
///
/// All content in this file is in the public domain. Use it any way you wish.

#pragma once

#include <stddef.h>
#include <stdio.h>

/// print data in binary format
///
/// @param data Start of data to print
/// @param size Number of bytes at `data`
/// @param stream Stream to write to
/// @return Number of characters printed on success or `EOF` on failure
int putb(const void *data, size_t size, FILE *stream);
