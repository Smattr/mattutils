/// @file
/// @brief Implementation of the hash.h API
///
/// All content in this file is in the public domain. Use it any way you wish.

#include <assert.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <ute/attr.h>
#include <ute/hash.h>

/// MurmurHash by Austin Appleby
///
/// This implementation of MurmurHash64A is based on the public domain reference
/// implementation with the following modifications:
///   • The seed is hard coded to 0; and
///   • Undefined Behaviour is avoided.
/// More information on MurmurHash at https://github.com/aappleby/smhasher/.
///
/// @param data Data to hash
/// @param len Number of bytes at `data`
/// @return A hash digest of the data
static uint64_t MurmurHash64A(const void *data, size_t len) {

  const uint64_t seed = 0;

  const uint64_t m = UINT64_C(0xc6a4a7935bd1e995);
  const unsigned r = 47;

  uint64_t h = seed ^ (len * m);

  const unsigned char *d = data;
  const unsigned char *end = d + len / sizeof(uint64_t) * sizeof(uint64_t);

  while (d != end) {

    uint64_t k;
    memcpy(&k, d, sizeof(k));
    d += sizeof(k);

    k *= m;
    k ^= k >> r;
    k *= m;

    h ^= k;
    h *= m;
  }

  switch (len & 7) {
  case 7:
    h ^= (uint64_t)d[6] << 48;
    FALLTHROUGH;
  case 6:
    h ^= (uint64_t)d[5] << 40;
    FALLTHROUGH;
  case 5:
    h ^= (uint64_t)d[4] << 32;
    FALLTHROUGH;
  case 4:
    h ^= (uint64_t)d[3] << 24;
    FALLTHROUGH;
  case 3:
    h ^= (uint64_t)d[2] << 16;
    FALLTHROUGH;
  case 2:
    h ^= (uint64_t)d[1] << 8;
    FALLTHROUGH;
  case 1:
    h ^= (uint64_t)d[0];
    h *= m;
  }

  h ^= h >> r;
  h *= m;
  h ^= h >> r;

  return h;
}

size_t hash(const void *data, size_t size) {
  assert(data != NULL || size == 0);
  return (size_t)MurmurHash64A(data, size);
}
