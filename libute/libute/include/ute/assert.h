/// @file
/// @brief More powerful assertions
///
/// All content in this file is in the public domain. Use it any way you wish.

#pragma once

#include <assert.h>
#include <ute/assume.h>

/// claim an expression is true
///
/// This is similar to `assert` but turns into an optimization hint in release
/// builds.
#ifdef NDEBUG
#define ASSERT(expr) ASSUME(expr)
#else
#define ASSERT(expr) assert(expr)
#endif
