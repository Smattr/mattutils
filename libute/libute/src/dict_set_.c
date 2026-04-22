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

/// allocate storage for a new dictionary box
///
/// @param alignment Required alignment for the box
/// @param size Required size in bytes
/// @return Pointer to new storage on success or `NULL` on out of memory
static void *alloc(size_t alignment, size_t size) {

  // ensure we never return null for a successful allocation
  if (size == 0)
    size = 1;

  // For small (in both size and alignment) allocations, `aligned_alloc` may
  // delegate to `malloc`. In this scenario, some allocators will return
  // pointers that are < 4 byte aligned, which also appears to be explicitly
  // allowed under ≥C23. We need ≥ 4 byte alignment (see ./dict.h), so be
  // explicit about this.
  // See also https://github.com/openjdk/jdk/pull/28235.
  if (alignment < 4) {
    alignment = 4;
    if (size % 4 != 0)
      size += 4 - size % 4;
  }

  return aligned_alloc(alignment, size);
}

static void dict_dtor(void *dict, void *context UNUSED) {
  assert(dict != NULL);

  dict_impl_t *const d = dict;

  // free our slots
  for (size_t i = 0; i < dict_capacity(*d); ++i) {
    const dword_t slot = slot_load(&d->base[i]);
    sp_t sp = slot_decode(slot);
    sp.ptr = slot_to_ptr(slot); // clear MIGRATED|DELETED
    sp_rel(sp);
  }

  free(d->base);
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

static int insert(dict_impl_t *dict, sp_t item, dict_sig_t_ sig) {
  assert(dict != NULL);

  const size_t h = (sig.hash != NULL ? sig.hash : hash)(item.ptr, sig.key_size);
  for (size_t i = 0; i < dict_capacity(*dict); ++i) {
    const size_t index = (h + i) % dict_capacity(*dict);
    dword_t slot = slot_load(&dict->base[index]);
  retry:

    // has someone else begun a migration?
    if (slot_is_moved(slot))
      return ENOMEM;

    // if this slot is unoccupied, try to insert our item
    if (slot_is_free(slot)) {
      if (!slot_cas(&dict->base[index], &slot, slot_encode(item)))
        goto retry;
      (void)atomic_fetch_add_explicit(&dict->used, 1, memory_order_acq_rel);
      return 0;
    }

    // if this is a deleted item, skip over it
    if (slot_is_deleted(slot))
      continue;

    // if this is our key, delete it
    // FIXME: This is not ideal because it leaves a window where the key is
    // _not_ present at all for other threads to observe. But I have not yet
    // come up with a better approach.
    void *const p = slot_to_ptr(slot);
    if (sig.key_size == 0 || memcmp(p, item.ptr, sig.key_size) == 0) {
      if (!slot_cas(&dict->base[index], &slot, slot_deleted(slot)))
        goto retry;
      (void)atomic_fetch_add_explicit(&dict->deleted, 1, memory_order_acq_rel);
      continue;
    }
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
    dword_t slot = slot_load(&src->base[i]);
  retry:

    // Did someone else beat us to migration? CASing in the “migrated” bit to
    // the first slot is how we authoritatively claim that we and only we are
    // migrating this dictionary, so we should only ever race with other
    // migrators on the first slot.
    if (slot_is_moved(slot)) {
      assert(i == 0 && "another migrator skipped the first slot");
      return EALREADY;
    }

    if (!slot_cas(&src->base[i], &slot, slot_moved(slot))) {
      // an inserter or deleter (or migrator if i == 0) beat us
      goto retry;
    }

    if (slot_is_free(slot) || slot_is_deleted(slot))
      continue;

    const sp_t item = slot_decode(slot);
    const sp_t copy = sp_dup(item);
    const int rc UNUSED = insert(dst, copy, sig);
    assert(rc == 0 && "rehash destination not owned exclusively?");
  }

  return 0;
}

int dict_set_(dict_t_ *dict, const void *key, const void *value,
              dict_sig_t_ sig) {
  assert(dict != NULL);
  assert(key != NULL || sig.key_size == 0);
  assert(value != NULL || sig.value_size == 0);

  // copy key and value for insertion
  void *const box = alloc(sig.witness_alignment, sig.witness_size);
  if (box == NULL)
    return ENOMEM;
  if (sig.key_size > 0)
    memcpy(box, key, sig.key_size);
  if (sig.value_size > 0)
    memcpy((char *)box + sig.witness_value_offset, value, sig.value_size);
  const sp_t copy = sp_new(box, slot_dtor, NULL);
  if (copy.ptr == NULL) {
    free(box);
    return ENOMEM;
  }

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
    atomic_dword_t *const b = aligned_calloc(alignof(atomic_dword_t),
                                             (size_t)1 << c >> 1, sizeof(b[0]));
    if (b == NULL) {
      sp_rel(sp);
      sp_rel(copy);
      return ENOMEM;
    }

    dict_impl_t *new = malloc(sizeof(*new));
    if (new == NULL) {
      free(b);
      sp_rel(sp);
      sp_rel(copy);
      return ENOMEM;
    }
    *new = (dict_impl_t){.base = b, .capacity = c};

    sp_t new_sp = sp_new(new, dict_dtor, NULL);
    if (new_sp.ptr == NULL) {
      dict_dtor(new, NULL);
      sp_rel(sp);
      sp_rel(copy);
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

  // insert the box
  {
    const int rc = insert(d, copy, sig);
    sp_rel(sp);
    if (rc != 0)
      goto retry;
  }

  return 0;
}
