/// @file
/// @brief 128-bit compare-and-swap does not write to expected on success
///
/// A subtle potential problem in CAS implementations is if the expected
/// location is written to even on success. On the surface, this seems fine. But
/// one of the common ways CAS is used is to implement algorithms that rely on
/// the destination and expected being components of the same shared data
/// structure. That is, the location of the expected value is shared with other
/// threads who are assuming they can write to it with ownership on seeing a
/// successful CAS. If the CAS itself writes to the expected location on
/// success, its write races with these updates coming from other threads. This
/// bug seems to have plagued all major libstdc++ implementations in the past:
///   •
///   https://stackoverflow.com/questions/21879331/is-stdatomic-compare-exchange-weak-thread-unsafe-by-design
///   • https://gcc.gnu.org/bugzilla/show_bug.cgi?id=60272
///   • https://bugs.llvm.org/show_bug.cgi?id=18899
///   • https://www.open-std.org/jtc1/sc22/wg21/docs/lwg-defects.html#2426
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <stdbool.h>
#include <sys/mman.h>
#include <unistd.h>
#include <ute/int128.h>

/// signed 128-bit CAS with a read-only expected
TEST("int128 CAS on read-only expected") {

  const size_t page_size = sysconf(_SC_PAGESIZE);
  void *const p = mmap(NULL, page_size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  ASSERT_NE(p, MAP_FAILED);

  // setup a CAS target and expected with the same value
  int128_t target = 42;
  int128_t *const expected = p;
  *expected = target;

  // make `expected` read-only
  {
    const int r = mprotect(p, page_size, PROT_READ);
    ASSERT_EQ(r, 0);
  }

  // CAS into it that should succeed
  const bool r = int128_atomic_cas(&target, expected, 43);
  ASSERT(r);

  (void)munmap(p, page_size);
}

/// unsigned 128-bit CAS with a read-only expected
TEST("uint128 CAS on read-only expected") {

  const size_t page_size = sysconf(_SC_PAGESIZE);
  void *const p = mmap(NULL, page_size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  ASSERT_NE(p, MAP_FAILED);

  // setup a CAS target and expected with the same value
  uint128_t target = 42;
  uint128_t *const expected = p;
  *expected = target;

  // make `expected` read-only
  {
    const int r = mprotect(p, page_size, PROT_READ);
    ASSERT_EQ(r, 0);
  }

  // CAS into it that should succeed
  const bool r = uint128_atomic_cas(&target, expected, 43);
  ASSERT(r);

  (void)munmap(p, page_size);
}
