/// @file
/// @brief Implementation of dictionary insertion
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "dict.h"
#include <assert.h>
#include <errno.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ute/asp.h>
#include <ute/attr.h>
#include <ute/dict.h>
#include <ute/hash.h>

/// `aligned_alloc` equivalent of `calloc`
static void *aligned_calloc(size_t alignment, size_t n, size_t size) {
  if (n > 0 && SIZE_MAX / n < size)
    return NULL;
  void *const p = aligned_alloc(alignment, n * size);
  if (p != NULL)
    memset(p, 0, n * size);
  return p;
}

/// allocate storage for a new dictionary value
///
/// @param alignment Required alignment for the value
/// @param size Required size in bytes
/// @return Pointer to new storage on success or `NULL` on out of memory
static void *alloc(size_t alignment, size_t size) {

  // ensure we never return null for a successful allocation
  if (size == 0)
    size = 1;

  // For small (in both size and alignment) allocations, `aligned_alloc` may
  // delegate to `malloc`. In this scenario, some allocators will return
  // pointers that are < 2 byte aligned, which also appears to be explicitly
  // allowed under ≥C23. We need ≥ 2 byte alignment (see ./dict.h), so be
  // explicit about this.
  // See also https://github.com/openjdk/jdk/pull/28235.
  if (alignment < 2) {
    alignment = 2;
    if (size % 2 != 0)
      size += 2 - size % 2;
  }

  return aligned_alloc(alignment, size);
}

static void dict_dtor(void *dict, void *context) {
  assert(dict != NULL);

  dict_impl_t *const d = dict;
  void (*value_dtor)(void *) = context;

  // free our keys
  for (size_t i = 0; i < dict_capacity(*d); ++i) {
    const dword_t k = key_slot_load(&d->key[i]);
    sp_t sp = key_slot_decode(k);
    sp.ptr = key_slot_to_ptr(k); // clear MIGRATED
    sp_rel(sp);
  }

  // free our values
  for (size_t i = 0; i < dict_capacity(*d); ++i) {
    const uintptr_t v = value_slot_load(&d->value[i]);
    void *const value = value_slot_to_ptr(v);
    if (value != NULL && value_dtor != NULL)
      value_dtor(value);
    free(value);
  }

  free(d->value);
  free(d->key);
  free(d);
}

/// deallocate a dictionary element that is going out of scope
///
/// @param ptr Pointer to the element
/// @param context Ignored
static void slot_dtor(void *ptr, void *context UNUSED) {
  assert(ptr != NULL);

  free(ptr);
}

static int insert(dict_impl_t *dict, sp_t key, void *value, dict_sig_t_ sig) {
  assert(dict != NULL);
  assert(key.ptr != NULL);
  assert(value != NULL);

  // has `key` been saved somewhere or `sp_rel`-ed?
  bool key_consumed = false;

  const size_t h = (sig.hash != NULL ? sig.hash : hash)(key.ptr, sig.key_size);
  for (size_t i = 0; i < dict_capacity(*dict); ++i) {
    const size_t index = (h + i) % dict_capacity(*dict);
    dword_t k = key_slot_load(&dict->key[index]);

  retry1:
    // if this slot is empty, try to claim it as ours
    if (key_slot_is_free(k)) {
      if (!key_slot_cas(&dict->key[index], &k, key_slot_encode(key)))
        goto retry1;
      // Note that we saved the key somewhere globally visible. This effectively
      // counts as our 1 reference. But it is fine to hang on to the
      // (semantically no longer accounted for) `key` because we hold a
      // reference to `dict`. This reference prevents the key we just wrote
      // being destructed.
      key_consumed = true;
      (void)atomic_fetch_add_explicit(&dict->used, 1, memory_order_acq_rel);

    } else {
      // if this slot is not ours, skip it
      const void *const p = key_slot_to_ptr(k);
      if (sig.key_size != 0 && memcmp(p, key.ptr, sig.key_size) != 0)
        continue;
    }

    // load the corresponding value slot
    uintptr_t v = value_slot_load(&dict->value[index]);

  retry2:
    // has someone else begun a migration?
    if (value_slot_is_moved(v)) {
      // If we are only implicitly holding a reference count for `key`, make
      // this explicit now. Our caller wants to retry on our failure and,
      // without this, will unknowingly be reusing a consumed pointer.
      if (key_consumed)
        (void)sp_dup(key);

      return ENOMEM;
    }

    // store our updated value
    if (!value_slot_cas(&dict->value[index], &v, (uintptr_t)value))
      goto retry2;

    // cleanup any value we just overwrote
    {
      void *const val = value_slot_to_ptr(v);
      if (val != NULL && sig.value_dtor != NULL)
        sig.value_dtor(val);
      free(val);
    }
    if (value_slot_is_free(v))
      (void)atomic_fetch_add_explicit(&dict->size, 1, memory_order_acq_rel);

    // if we did not use the key, discard it
    if (!key_consumed)
      sp_rel(key);

    return 0;
  }

  return ENOMEM;
}

static int rehash(dict_impl_t *dst, dict_impl_t *src, dict_sig_t_ sig) {
  assert(dst != NULL);
  assert(src == NULL || dict_capacity(*dst) >= dict_capacity(*src));

  // nothing to do for an uninitialised dictionary
  if (src == NULL)
    return 0;

  for (size_t i = 0; i < dict_capacity(*src); ++i) {
    uintptr_t v = value_slot_load(&src->value[i]);
  retry:

    // Did someone else beat us to migration? CASing in the “migrated” bit to
    // the first slot is how we authoritatively claim that we and only we are
    // migrating this dictionary, so we should only ever race with other
    // migrators on the first slot.
    if (value_slot_is_moved(v)) {
      assert(i == 0 && "another migrator skipped the first slot");
      return EALREADY;
    }

    // mark this slot as migrated
    if (!value_slot_cas(&src->value[i], &v, value_slot_moved(0))) {
      // an inserter or deleter (or migrator if i == 0) beat us
      goto retry;
    }

    if (value_slot_is_free(v))
      continue;

    const dword_t k = key_slot_load(&src->key[i]);

    const sp_t item = key_slot_decode(k);
    const sp_t copy = sp_dup(item);
    const int rc UNUSED = insert(dst, copy, value_slot_to_ptr(v), sig);
    assert(rc == 0 && "rehash destination not owned exclusively?");
  }

  return 0;
}

int dict_set_(dict_t_ *dict, const void *key, void *value, dict_sig_t_ sig) {
  assert(dict != NULL);
  assert(key != NULL || sig.key_size == 0);
  assert(value != NULL || sig.value_size == 0);

  // copy key for insertion
  const size_t k_size = sig.key_size == 0 ? 1 : sig.key_size;
  void *const box = aligned_alloc(sig.key_alignment, k_size);
  if (box == NULL) {
    if (sig.value_dtor != NULL)
      sig.value_dtor(value);
    return ENOMEM;
  }
  if (sig.key_size > 0)
    memcpy(box, key, sig.key_size);
  const sp_t k = sp_new(box, slot_dtor, NULL);
  if (k.ptr == NULL) {
    free(box);
    if (sig.value_dtor != NULL)
      sig.value_dtor(value);
    return ENOMEM;
  }

  // copy value for insertion, noting that we need a non-null pointer
  void *const v = alloc(sig.value_alignment, sig.value_size);
  if (v == NULL) {
    sp_rel(k);
    if (sig.value_dtor != NULL)
      sig.value_dtor(value);
    return ENOMEM;
  }
  if (sig.value_size > 0)
    memcpy(v, value, sig.value_size);

  // percentage occupancy at which we expand the backing storage
  enum { LOAD_FACTOR = 70 };

retry:;

  // acquire a reference to the dictionary
  sp_t sp = sp_acq(&dict->root);

  dict_impl_t *const d = sp.ptr;
  const size_t used =
      d == NULL ? 0 : atomic_load_explicit(&d->used, memory_order_acquire);
  const size_t capacity = d == NULL ? 0 : dict_capacity(*d);

  // do we need to expand the backing storage?
  if (used * 100 >= capacity * LOAD_FACTOR) {
    const size_t c = capacity + 1;

    atomic_dword_t *const ks = aligned_calloc(
        alignof(atomic_dword_t), (size_t)1 << c >> 1, sizeof(ks[0]));
    if (ks == NULL) {
      sp_rel(sp);
      if (sig.value_dtor != NULL)
        sig.value_dtor(v);
      free(v);
      sp_rel(k);
      return ENOMEM;
    }

    atomic_uintptr_t *const vs = calloc((size_t)1 << c >> 1, sizeof(vs[0]));
    if (vs == NULL) {
      free(ks);
      sp_rel(sp);
      if (sig.value_dtor != NULL)
        sig.value_dtor(v);
      free(v);
      sp_rel(k);
      return ENOMEM;
    }

    dict_impl_t *new = malloc(sizeof(*new));
    if (new == NULL) {
      free(vs);
      free(ks);
      sp_rel(sp);
      if (sig.value_dtor != NULL)
        sig.value_dtor(v);
      free(v);
      sp_rel(k);
      return ENOMEM;
    }
    *new = (dict_impl_t){.key = ks, .value = vs, .capacity = c};

    sp_t new_sp = sp_new(new, dict_dtor, sig.value_dtor);
    if (new_sp.ptr == NULL) {
      dict_dtor(new, sig.value_dtor);
      sp_rel(sp);
      if (sig.value_dtor != NULL)
        sig.value_dtor(v);
      free(v);
      sp_rel(k);
      return ENOMEM;
    }

    if (rehash(new, d, sig) != 0) {
      sp_rel(new_sp);
      sp_rel(sp);
      goto retry;
    }

    const bool r = sp_cas(&dict->root, sp, new_sp);
    assert((r || sp.ptr == NULL) && "successful migrations race one another");
    if (!r)
      sp_rel(new_sp);
    sp_rel(sp);
    goto retry;
  }

  // insert the key+value
  {
    const int rc = insert(d, k, v, sig);
    sp_rel(sp);
    if (rc != 0)
      goto retry;
  }

  return 0;
}
