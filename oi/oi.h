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
///
/// If you are trying to understand the implementation below, start with the
/// `oi` macro and work backwards from there.

#pragma once

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

/// a value of arbitrary type
///
/// It is assumed there is an external way of discriminating which of the union
/// members is in use.
struct oi_value_ {
  union {
    long long signed_value;
    unsigned long long unsigned_value;
    double double_value;
    const char *char_ptr_value;
  };
};

/// construct a `oi_value_` with a signed value
///
/// This function is not expected to be called directly by users. It is only
/// expected to be called from the `oi` macro.
static inline struct oi_value_ oi_make_signed_(long long value) {
  struct oi_value_ v;
  v.signed_value = value;
  return v;
}

/// construct a `oi_value_` with an unsigned value
///
/// This function is not expected to be called directly by users. It is only
/// expected to be called from the `oi` macro.
static inline struct oi_value_ oi_make_unsigned_(unsigned long long value) {
  struct oi_value_ v;
  v.unsigned_value = value;
  return v;
}

/// construct a `oi_value_` with a double value
///
/// This function is not expected to be called directly by users. It is only
/// expected to be called from the `oi` macro.
static inline struct oi_value_ oi_make_double_(double value) {
  struct oi_value_ v;
  v.double_value = value;
  return v;
}

/// construct a `oi_value_` with a string value
///
/// This function is not expected to be called directly by users. It is only
/// expected to be called from the `oi` macro.
static inline struct oi_value_ oi_make_char_ptr_(const char *value) {
  struct oi_value_ v;
  v.char_ptr_value = value;
  return v;
}

/// construct a `oi_value_`
///
/// These functions are not expected to be called directly by users. They are
/// only expected to be called from the `oi` macro.
#ifdef __cplusplus
static inline oi_value_ oi_make_value_(signed char v) {
  return oi_make_signed_(v);
}

static inline oi_value_ oi_make_value_(short v) { return oi_make_signed_(v); }

static inline oi_value_ oi_make_value_(int v) { return oi_make_signed_(v); }

static inline oi_value_ oi_make_value_(long v) { return oi_make_signed_(v); }

static inline oi_value_ oi_make_value_(long long v) {
  return oi_make_signed_(v);
}

static inline oi_value_ oi_make_value_(unsigned char v) {
  return oi_make_unsigned_(v);
}

static inline oi_value_ oi_make_value_(unsigned short v) {
  return oi_make_unsigned_(v);
}

static inline oi_value_ oi_make_value_(unsigned v) {
  return oi_make_unsigned_(v);
}

static inline oi_value_ oi_make_value_(unsigned long v) {
  return oi_make_unsigned_(v);
}

static inline oi_value_ oi_make_value_(unsigned long long v) {
  return oi_make_unsigned_(v);
}

static inline oi_value_ oi_make_value_(float v) { return oi_make_double_(v); }

static inline oi_value_ oi_make_value_(double v) { return oi_make_double_(v); }

static inline oi_value_ oi_make_value_(const char *v) {
  return oi_make_char_ptr_(v);
}
#else
#define oi_make_value_(v)                                                      \
  (_Generic((v),                                                               \
       signed char: oi_make_signed_,                                           \
       short: oi_make_signed_,                                                 \
       int: oi_make_signed_,                                                   \
       long: oi_make_signed_,                                                  \
       long long: oi_make_signed_,                                             \
       unsigned char: oi_make_unsigned_,                                       \
       unsigned short: oi_make_unsigned_,                                      \
       unsigned: oi_make_unsigned_,                                            \
       unsigned long: oi_make_unsigned_,                                       \
       unsigned long long: oi_make_unsigned_,                                  \
       float: oi_make_double_,                                                 \
       double: oi_make_double_,                                                \
       char *: oi_make_char_ptr_,                                              \
       const char *: oi_make_char_ptr_)(v))
#endif

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

// `oi_print_`, `oi_signed_`, `oi_unsigned_`, `oi_double_`, and `oi_char_ptr_`
// are expected to conform to the same type signature. This is necessary to be
// able to branch to them from the `oi` macro.

/// do a debugging print
///
/// This function is not expected to be called directly by users. It is only
/// expected to be called from the `oi` macro.
///
/// @param format A printf-style format string
/// @param ap printf-style format arguments
static inline __attribute__((format(printf, 1, 0))) void
oi_vprint_(const char *format, va_list ap) {
  oi_printed_ = 1;

  int use_colour = 0;
#ifdef __has_include
#if __has_include(<unistd.h>)
  use_colour = isatty(STDERR_FILENO);
#endif
#endif

#if defined(__GNUC__) && !defined(__cplusplus)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnested-externs"
#endif
  extern void flockfile(FILE *);
#if defined(__GNUC__) && !defined(__cplusplus)
#pragma GCC diagnostic pop
#endif
  flockfile(stderr);

  const time_t now = time(NULL);
  const struct tm *const now_tm = localtime(&now);
  fprintf(stderr, "%s[OI %04d-%02d-%02d %02d:%02d] %s:%d: ",
          use_colour ? "\033[33;1m" : "", now_tm->tm_year + 1900,
          now_tm->tm_mon + 1, now_tm->tm_mday, now_tm->tm_hour, now_tm->tm_sec,
          oi_filename_, oi_lineno_);
  vfprintf(stderr, format, ap);
  fprintf(stderr, "%s\n", use_colour ? "\033[0m" : "");

#if defined(__GNUC__) && !defined(__cplusplus)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnested-externs"
#endif
  extern void funlockfile(FILE *);
#if defined(__GNUC__) && !defined(__cplusplus)
#pragma GCC diagnostic pop
#endif
  funlockfile(stderr);
}

/// do a debugging print
///
/// This function is not expected to be called directly by users. It is only
/// expected to be called from the `oi` macro.
///
/// @param ignored This parameter is unused
/// @param format A printf-style format string
/// @param ignored2 This parameter is unused
/// @param ... printf-style format arguments
static inline __attribute__((format(printf, 2, 4))) void
oi_print_(const char *ignored, const char *format, struct oi_value_ ignored2,
          ...) {
  (void)ignored;

  va_list ap;
  va_start(ap, ignored2);

  oi_vprint_(format, ap);

  va_end(ap);
}

/// print a signed value for debugging
///
/// This function is not expected to be called directly by users. It is only
/// expected to be called from the `oi` macro.
///
/// @param name Symbol or expression from which this value originated
/// @param ignored This parameter is unused
/// @param value Value to print
/// @param ... These parameters are unused
static inline void oi_signed_(const char *name, const char *ignored,
                              struct oi_value_ value, ...) {
  (void)ignored;

  assert(name != NULL);
  assert(strcmp(name, "") != 0);

  oi_print_(NULL, "%s == %lld", value, name, value.signed_value);

  // if this could be character data, print its equivalent
  if (value.signed_value > 31 && value.signed_value < 127)
    oi_print_(NULL, "(%s == '%c')", value, name, (char)value.signed_value);
}

/// print an unsigned value for debugging
///
/// This function is not expected to be called directly by users. It is only
/// expected to be called from the `oi` macro.
///
/// @param name Symbol or expression from which this value originated
/// @param ignored This parameter is unused
/// @param value Value to print
/// @param ... These parameters are unused
static inline void oi_unsigned_(const char *name, const char *ignored,
                                struct oi_value_ value, ...) {
  (void)ignored;

  assert(name != NULL);
  assert(strcmp(name, "") != 0);

  oi_print_(NULL, "%s == %llu, 0x%llx", value, name, value.unsigned_value,
            value.unsigned_value);

  // if this could be character data, print its equivalent
  if (value.unsigned_value > 31 && value.unsigned_value < 127)
    oi_print_(NULL, "(%s == '%c')", value, name, (char)value.unsigned_value);
}

/// print a double value for debugging
///
/// This function is not expected to be called directly by users. It is only
/// expected to be called from the `oi` macro.
///
/// @param name Symbol or expression from which this value originated
/// @param ignored This parameter is unused
/// @param value Value to print
/// @param ... These parameters are unused
static inline void oi_double_(const char *name, const char *ignored,
                              struct oi_value_ value, ...) {
  (void)ignored;

  assert(name != NULL);
  assert(strcmp(name, "") != 0);

  oi_print_(NULL, "%s == %f", value, name, value.double_value);
}

/// print a string value for debugging
///
/// This function is not expected to be called directly by users. It is only
/// expected to be called from the `oi` macro.
///
/// @param name Symbol or expression from which this value originated
/// @param ignored This parameter is unused
/// @param value Value to print
/// @param ... These parameters are unused
static inline void oi_char_ptr_(const char *name, const char *ignored,
                                struct oi_value_ value, ...) {
  (void)ignored;

  assert(name != NULL);
  assert(strcmp(name, "") != 0);

  oi_print_(NULL, "%s == \"%s\"", value, name, value.char_ptr_value);
}

/// tear down from a debugging print
///
/// This function is not expected to be called directly by users. It is only
/// expected to be called from the `oi` macro.
static inline void oi_close_(void) {
  if (oi_printed_ == 0) {
    struct oi_value_ ignored;
    ignored.signed_value = 0;
    oi_print_(NULL, "here", ignored);
  }
  oi_printed_ = 0;
}

/// no-op
///
/// This function exists purely to allow macro expansions that result in
/// parameter-less expressions `oi__` to be syntactically valid.
static inline void oi__(void) {}

/// make a debugging print
///
/// The strange form of this macro is intended to support three use cases:
///
///   oi("my %s", "message"); // printf-style debugging
///   oi; // just print “here” with file and line number information
///   oi(x); // print “x == <value of x>”
#define oi                                                                     \
  for (int oi_ = (oi_open_(__FILE__, __LINE__), 1); oi_; oi_close_(), --oi_)   \
  (void)oi__

/// select an expression printer
///
/// This is not expected to be called directly by users. It is only expected to
/// be called from the `oi` macro.
#define oi_expr_(control)                                                      \
  _Generic((control),                                                          \
      signed char: oi_signed_,                                                 \
      short: oi_signed_,                                                       \
      int: oi_signed_,                                                         \
      long: oi_signed_,                                                        \
      long long: oi_signed_,                                                   \
      unsigned char: oi_unsigned_,                                             \
      unsigned short: oi_unsigned_,                                            \
      unsigned: oi_unsigned_,                                                  \
      unsigned long: oi_unsigned_,                                             \
      unsigned long long: oi_unsigned_,                                        \
      float: oi_double_,                                                       \
      double: oi_double_,                                                      \
      char *: oi_char_ptr_,                                                    \
      const char *: oi_char_ptr_)

/// return a format string, if we were given one, else a stub substitute
#ifdef __cplusplus
template <typename T> static inline constexpr const char *oi_fmt_(T) {
  return "unused";
}
template <> inline constexpr const char *oi_fmt_(const char fmt[]) {
  return fmt;
}
#else
#define oi_fmt_(control)                                                       \
  __builtin_choose_expr(                                                       \
      __builtin_types_compatible_p(__typeof__(control), char[]), (control),    \
      "unused")
#endif

#ifdef __cplusplus
/// call another `oi_*` function based on the template parameters
template <bool do_print, typename T>
static inline void oi_print_expr_(const char *expr, const char *fmt, T value,
                                  ...);

template <>
inline __attribute__((format(printf, 2, 4))) void
oi_print_expr_<true>(const char *, const char *format, const char *value, ...) {
  va_list ap;
  va_start(ap, value);

  oi_vprint_(format, ap);

  va_end(ap);
}

template <>
inline void oi_print_expr_<false>(const char *expr, const char *,
                                  signed char value, ...) {
  oi_signed_(expr, nullptr, oi_make_value_(value));
}
template <>
inline void oi_print_expr_<false>(const char *expr, const char *, short value,
                                  ...) {
  oi_signed_(expr, nullptr, oi_make_value_(value));
}
template <>
inline void oi_print_expr_<false>(const char *expr, const char *, int value,
                                  ...) {
  oi_signed_(expr, nullptr, oi_make_value_(value));
}
template <>
inline void oi_print_expr_<false>(const char *expr, const char *, long value,
                                  ...) {
  oi_signed_(expr, nullptr, oi_make_value_(value));
}
template <>
inline void oi_print_expr_<false>(const char *expr, const char *,
                                  long long value, ...) {
  oi_signed_(expr, nullptr, oi_make_value_(value));
}
template <>
inline void oi_print_expr_<false>(const char *expr, const char *,
                                  unsigned char value, ...) {
  oi_unsigned_(expr, nullptr, oi_make_value_(value));
}
template <>
inline void oi_print_expr_<false>(const char *expr, const char *,
                                  unsigned short value, ...) {
  oi_unsigned_(expr, nullptr, oi_make_value_(value));
}
template <>
inline void oi_print_expr_<false>(const char *expr, const char *,
                                  unsigned value, ...) {
  oi_unsigned_(expr, nullptr, oi_make_value_(value));
}
template <>
inline void oi_print_expr_<false>(const char *expr, const char *,
                                  unsigned long value, ...) {
  oi_unsigned_(expr, nullptr, oi_make_value_(value));
}
template <>
inline void oi_print_expr_<false>(const char *expr, const char *,
                                  unsigned long long value, ...) {
  oi_unsigned_(expr, nullptr, oi_make_value_(value));
}
template <>
inline void oi_print_expr_<false>(const char *expr, const char *, float value,
                                  ...) {
  oi_double_(expr, nullptr, oi_make_value_(value));
}
template <>
inline void oi_print_expr_<false>(const char *expr, const char *, double value,
                                  ...) {
  oi_double_(expr, nullptr, oi_make_value_(value));
}
template <>
inline void oi_print_expr_<false>(const char *expr, const char *,
                                  const char *value, ...) {
  oi_char_ptr_(expr, nullptr, oi_make_value_(value));
}
#endif

#ifdef __cplusplus
#define oi__(fmt, ...)                                                         \
  oi_print_expr_<#fmt[0] == '"'>(#fmt, oi_fmt_(fmt), fmt, ##__VA_ARGS__)
#else
#define oi__(fmt, ...)                                                         \
  ((#fmt[0] == '"' ? oi_print_ : oi_expr_(fmt))(                               \
      #fmt, oi_fmt_(fmt), oi_make_value_(fmt), ##__VA_ARGS__))
#endif

#ifdef __has_include
#if __has_include(<execinfo.h>)
#include <execinfo.h>
#endif
#endif

/// print a backtrace of the caller’s location
///
/// This function is not expected to be called directly by users. It is only
/// expected to be called from the `oi_bt` macro.
///
/// @param filename Source file of the caller
/// @param lineno Source line number of the caller
static inline __attribute__((always_inline)) void oi_bt_(const char *filename,
                                                         int lineno) {

  oi_open_(filename, lineno);

  // Glibc backtrace support
#if defined(__GNUC__) && !defined(__cplusplus)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnested-externs"
#endif
  extern int backtrace(void **, int) __attribute((weak));
  extern char **backtrace_symbols(void *const *buffer, int)
      __attribute__((weak));
#if defined(__GNUC__) && !defined(__cplusplus)
#pragma GCC diagnostic pop
#endif

  // give up if we do not have backtrace support
  if (backtrace == NULL || backtrace_symbols == NULL) {
    oi__("backtraces unavailable");
    oi_close_();
    return;
  }

  void **buffer = NULL;
  int size = 0;
  int seen = 0;

  // read our backtrace
  while (size == seen) {
    size = size == 0 ? 128 : size * 2;
    void **const b = (void **)realloc(buffer, (size_t)size);
    if (b == NULL) {
      free(buffer);
      oi__("out of memory");
      oi_close_();
      return;
    }
    buffer = b;

    seen = backtrace(buffer, size);
  }

  // translate it into something human readable
  char **const names = backtrace_symbols(buffer, seen);
  free(buffer);
  if (names == NULL) {
    oi__("out of memory");
    oi_close_();
    return;
  }

  // dump it
  oi__("backtrace (use -rdynamic to GCC to get function names):");
  for (int i = 0; i < seen; ++i)
    oi__(" %s", names[i]);

  free(names);
  oi_close_();

  return;

  // dead code to test all relevant `oi` variants compile

  oi;
  oi("hello world");
  oi("hello %s", "world");

  {
    signed char x = 42;
    oi(x);
  }
  {
    short x = 42;
    oi(x);
  }
  {
    int x = 42;
    oi(x);
  }
  {
    long x = 42;
    oi(x);
  }
  {
    long long x = 42;
    oi(x);
  }
  {
    unsigned char x = 42;
    oi(x);
  }
  {
    unsigned short x = 42;
    oi(x);
  }
  {
    unsigned x = 42;
    oi(x);
  }
  {
    unsigned long x = 42;
    oi(x);
  }
  {
    unsigned long long x = 42;
    oi(x);
  }
  {
    float x = 42;
    oi(x);
  }
  {
    double x = 42;
    oi(x);
  }
  {
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
#ifndef __cplusplus
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
#endif
#endif
    char *x = "hello world";
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
    oi(x);
  }
  {
    const char *x = "hello world";
    oi(x);
  }
}

/// print a backtrace of the current location
///
/// If backtraces are unavailable, this prints an error and continues.
#define oi_bt() oi_bt_(__FILE__, __LINE__)
