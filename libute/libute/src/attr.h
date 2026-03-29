/// @file
/// @brief Compiler attribute abstractions
///
/// All content in this file is in the public domain. Use it any way you wish.

#pragma once

/// mark a symbol that should not be exported to library users
#if !defined(PRIVATE) && defined(__GNUC__)
#define PRIVATE __attribute__((visibility("hidden")))
#endif
#ifndef PRIVATE
#define PRIVATE /* nothing, for toolchains that do not support visibility */
#endif
