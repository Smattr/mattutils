/// @file
/// @brief scaffolding for printf-style debugging
///
/// When doing printf-debugging, you frequently spend time implementing the same
/// boring pieces of plumbing:
///   1. __FILE__ and __LINE__ in the output messages
///   2. Mutual exclusion if you are within a multithreaded program
/// This header aims to give you a ready-to-go flexible implementation optimised
/// for typing few characters (`oi` is very short) and debugging faster. See the
/// adjacent `oi` script for an easy way of including this in compilation.

#pragma once

#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>

// only include unistd.h conditionally so we have a hope of compiling on
// platforms without it
#ifdef __has_include
#if __has_include(<unistd.h>)
#include <unistd.h>
#endif
#endif

/// has the user called `oi_open_` but not yet called `oi_print_`?
static __thread int oi_printed_;

/// location of the current `oi` call
static __thread const char *oi_filename_;
static __thread int oi_lineno_;

/// setup for a debugging print
///
/// This function is not expected to be called directly by users. It is only
/// expected to be called from the `oi` macro.
///
/// @param filename Source file of an `oi` call site
/// @param line Source line of an `oi` call site
static inline void oi_open_(const char *filename, int lineno) {
  assert(oi_printed_ == 0);
  oi_filename_ = filename;
  oi_lineno_ = lineno;
}

/// do a debugging print
///
/// This function is not expected to be called directly by users. It is only
/// expected to be called from the `oi` macro.
///
/// @param format A printf-style format string
/// @param ... printf-style format arguments
static inline __attribute__((format(printf, 1, 2))) void
oi_print_(const char *format, ...) {
  assert(oi_printed_ == 0);
  oi_printed_ = 1;

  va_list ap;
  va_start(ap, format);

  int use_colour = 0;
#ifdef __has_include
#if __has_include(<unistd.h>)
  use_colour = isatty(STDERR_FILENO);
#endif
#endif

  extern void flockfile(FILE *);
  flockfile(stderr);

  const time_t now = time(NULL);
  const struct tm *const now_tm = localtime(&now);
  fprintf(stderr, "%s[OI %04d-%02d-%02d %02d:%02d] %s:%d: ",
          use_colour ? "\033[33;1m" : "", now_tm->tm_year + 1900,
          now_tm->tm_mon + 1, now_tm->tm_mday, now_tm->tm_hour, now_tm->tm_sec,
          oi_filename_, oi_lineno_);
  vfprintf(stderr, format, ap);
  fprintf(stderr, "%s\n", use_colour ? "\033[0m" : "");

  extern void funlockfile(FILE *);
  funlockfile(stderr);

  va_end(ap);
}

/// tear down from a debugging print
///
/// This function is not expected to be called directly by users. It is only
/// expected to be called from the `oi` macro.
static inline void oi_close_(void) {
  if (oi_printed_ == 0) {
    oi_print_("here");
  }
  oi_printed_ = 0;
}

/// make a debugging print
///
/// The strange form of this macro is intended to support two use cases:
///
///   oi("my %s", "message"); // printf-style debugging
///   oi; // just print “here” with file and line number information
#define oi                                                                     \
  for (int oi_ = (oi_open_(__FILE__, __LINE__), 1); oi_; oi_close_(), --oi_)   \
  (void)oi_print_

#define oi_show(expr)                                                          \
  oi(_Generic((expr),                                                          \
         const char *: #expr " == \"%s\"",                                     \
         int: #expr " == %d",                                                  \
         unsigned: #expr " == %u",                                             \
         long: #expr " == %ld",                                                \
         unsigned long: #expr " == %lu",                                       \
         long long: #expr " == %lld",                                          \
         unsigned long long: #expr " == %llu",                                 \
         float: #expr " == %f",                                                \
         double: #expr " == %f"),                                              \
     (expr))
