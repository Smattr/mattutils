/// @file
/// @brief Implementation of aligned allocation functions
///
/// All content in this file is in the public domain. Use it any way you wish.

#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ute/aligned_alloc.h>
#include <ute/attr.h>

#ifdef _MSC_VER
#include <malloc.h>
#endif

/// description of a live allocation
typedef struct {
  void *ptr;      ///< base of memory that was allocated
  char *filename; ///< source location of the originating `ALIGNED_ALLOC` call
  int lineno; ///< source line number of the originating `ALIGNED_ALLOC` call
} alloc_record_t;

/// live allocations
static alloc_record_t *allocated;
static size_t n_allocated;
static size_t c_allocated;

/// mutual exclusion mechanism for accessing `allocated`
static atomic_flag allocated_lock;

/// acquire exclusive access to `allocated`
static void lock(void) {
  while (
      atomic_flag_test_and_set_explicit(&allocated_lock, memory_order_acq_rel))
    ;
}

/// release exclusive access to `allocated`
static void unlock(void) {
  atomic_flag_clear_explicit(&allocated_lock, memory_order_release);
}

#ifndef NDEBUG
/// check for any memory leaks on exit
static __attribute__((destructor)) void leak_check(void) {
  lock();

  // are there any remaining `allocated` entries?
  for (size_t i = 0; i < n_allocated; ++i)
    fprintf(
        stderr,
        "%s:%d: allocated pointer %p was never freed through aligned_free_\n",
        allocated[i].filename, allocated[i].lineno, allocated[i].ptr);
  const bool fail = n_allocated > 0;

  // avoid ASan counting the allocated array itself as a leak
  for (size_t i = 0; i < n_allocated; ++i)
    free(allocated[i].filename);
  free(allocated);
  allocated = NULL;
  n_allocated = 0;
  c_allocated = 0;

  unlock();

  if (fail)
    abort();
}
#endif

static void aligned_free_core(void *ptr) {
#ifdef _MSC_VER
  _aligned_free(ptr);
#else
  free(ptr);
#endif
}

void *aligned_alloc_(size_t alignment, size_t size, const char *filename,
                     int lineno) {
  assert(filename != NULL);

#ifdef _MSC_VER
  void *const p = _aligned_malloc(size, alignment);
#else
  void *const p = aligned_alloc(alignment, size);
#endif

#ifdef NDEBUG
  const bool leak_checks = false;
#else
  const bool leak_checks = true;
#endif

  // record this allocation
  if (leak_checks && p != NULL) {
    const alloc_record_t r = {
        .ptr = p, .filename = strdup(filename), .lineno = lineno};
    if (r.filename == NULL) {
      aligned_free_core(p);
      return NULL;
    }

    lock();

    // do we need to expand `allocated`?
    if (n_allocated == ((size_t)1 << c_allocated >> 1)) {
      const size_t c = c_allocated + 1;
      alloc_record_t *const a =
          realloc(allocated, ((size_t)1 << c >> 1) * sizeof(a[0]));
      if (a == NULL) {
        free(r.filename);
        aligned_free_core(p);
        unlock();
        return NULL;
      }

      allocated = a;
      c_allocated = c;
    }

    // append this record
    allocated[n_allocated] = r;
    ++n_allocated;

    unlock();
  }

  return p;
}

void aligned_free_(void *ptr, const char *filename, int lineno) {
  assert(filename != NULL);

#ifdef NDEBUG
  const bool leak_checks = false;
#else
  const bool leak_checks = true;
#endif

  // check this was something we ourselves allocated
  if (leak_checks && ptr != NULL) {
    lock();

    // find and remove the record for this allocation
    bool found = false;
    for (size_t i = 0; i < n_allocated; ++i) {
      if (allocated[i].ptr != ptr)
        continue;
      free(allocated[i].filename);
      for (size_t j = i; j + 1 < n_allocated; ++j)
        allocated[j] = allocated[j + 1];
      --n_allocated;
      found = true;
      break;
    }

    unlock();

    if (!found) {
      fprintf(
          stderr,
          "%s:%d: freeing %p that was not allocated through aligned_alloc_\n",
          filename, lineno, ptr);
      abort();
    }
  }

  aligned_free_core(ptr);
}
