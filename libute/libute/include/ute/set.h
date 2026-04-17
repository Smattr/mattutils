/// @file
/// @brief Type-generic set
///
/// This set is:
///   • Type-generic – works for any element type (WIP)
///   • Type-safe – compiler should catch all incorrect parameter passing
///   • Thread-safe – all macros are safe to call concurrently
///   • Lock-free – no mutexes or semaphores involved
///   • Memory efficient – compile-time specialisation based on set element type
///
/// The trade off in being such a general belt-and-suspenders implementation is
/// that it is not particularly fast. But it still aims to be memory efficient.
///
/// The type-generic, type-safe aspect of this is based on techniques from:
///   Type Safe Generic Data Structures in C
///   Daniel Hooper
///   https://danielchasehooper.com/posts/typechecked-generic-c-data-structures
///
/// The thread-safe aspect of the “boxed” and “unboxed” set implementations is
/// based on techniques from:
///   A Lock-Free Wait-Free Hash Table
///   Dr Cliff Click
///   https://web.stanford.edu/class/ee380/Abstracts/070221_LockFreeHash.pdf
///
/// TODO:
///   • support char *
///
/// All content in this file is in the public domain. Use it any way you wish.

#pragma once

#include <limits.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <ute/asp.h>
#include <ute/typeof.h>

#ifdef __cplusplus
extern "C" {
#endif

/// a set containing items of a given type
///
/// This expands to a type that is intended to be zero-initialised:
///
///   SET(int) ints = {0};
///
/// In order to add custom member functions, you need to pass an extra `1`:
///
///   SET(int, 1) = {.dtor = {my_dtor}};
///
/// @param type Type of items that will be stored in the set
#define SET(type, ...)                                                         \
  struct {                                                                     \
    union {                                                                    \
      set_t_ impl; /**< private implementation */                              \
                                                                               \
      /** mechanism for re-obtaining the set item type                      */ \
      /*                                                                    */ \
      /* To achieve type safety, we need to be able to refer back to `type` */ \
      /* at call sites involving variables of the struct type being defined */ \
      /* here. To do this we propagate the type through an unused member.   */ \
      /* This does not need to be a pointer or be unioned with the `impl`   */ \
      /* member, but this ensures we minimise the size of the struct.       */ \
      type *witness;                                                           \
    };                                                                         \
                                                                               \
    /** optional user-supplied item hash                                    */ \
    /*                                                                      */ \
    /* If this member is non-zero sized and not null, it will be called     */ \
    /* when an item needs to be hashed.                                     */ \
    size_t (*hash[sizeof((int[]){__VA_ARGS__}) / sizeof(int)])(const void *,   \
                                                               size_t);        \
                                                                               \
    /** optional user-supplied item comparator                              */ \
    /*                                                                      */ \
    /* If this member is non-zero sized and not null, it will be called     */ \
    /* when two items need to be compared.                                  */ \
    bool (*eq[sizeof((int[]){__VA_ARGS__}) / sizeof(int)])(const void *,       \
                                                           const void *,       \
                                                           size_t);            \
                                                                               \
    /** optional user-supplied item destructor                              */ \
    /*                                                                      */ \
    /* If this member is non-zero sized and not null, it will be called on  */ \
    /* set items immediately before they are removed from the set.          */ \
    void (*dtor[sizeof((int[]){__VA_ARGS__}) / sizeof(int)])(void *);          \
  }

/// insert an item into a set
///
/// This macro can be thought of as having one of the C types:
///
///   int SET_INSERT(SET(<type>) *set, const <type> item);
///   int SET_INSERT(SET(<type>) *set, const <type> item, bool *exists);
///
/// `item` is “consumed” regardless of whether insertion is successful. This
/// only matters if you have set the `dtor` member of the set. That is, the
/// caller is responsible for eventually calling the destructor on the passed
/// item, whether the insertion succeeds or fails.
///
/// @param set Set to operate on
/// @param item Item to insert
/// @param exists [out] If not null, on success this will be set to whether the
///   item already existed in the set before the insertion
/// @return 0 on success or an errno on failure
#define SET_INSERT(set, item, ...)                                             \
  (SET_CAN_INLINE_(set)   ? set_inline_insert_                                 \
   : SET_CAN_BITSET_(set) ? set_bitset_insert_                                 \
   : SET_CAN_UNBOX_(set)  ? set_unboxed_insert_                                \
                          : set_boxed_insert_)(                                 \
      &(set)->impl, (TYPEOF(*(set)->witness)[1]){item},                        \
      (bool *[2]){NULL, ##__VA_ARGS__}[1], SET_SIG_(set))

/// remove an item from a set
///
/// This macro can be thought of as having the C type:
///
///   bool SET_REMOVE(SET(<type>) *set, const <type> item);
///
/// @param set Set to operate on
/// @param item Item to remove
/// @return True if the item was removed or false if it was not in the set
#define SET_REMOVE(set, item)                                                  \
  (SET_CAN_INLINE_(set)   ? set_inline_remove_                                 \
   : SET_CAN_BITSET_(set) ? set_bitset_remove_                                 \
   : SET_CAN_UNBOX_(set)  ? set_unboxed_remove_                                \
                          : set_boxed_remove_)(                                 \
      &(set)->impl, (TYPEOF(*(set)->witness)[1]){item}, SET_SIG_(set))

/// does an item exist in a set?
///
/// This macro can be thought of as having the C type:
///
///   bool SET_CONTAINS(SET(<type>) *set, const <type> item);
///
/// @param set Set to operate on
/// @param item Item whose existence to check
/// @return True if the item was found in the set
#define SET_CONTAINS(set, item)                                                \
  (SET_CAN_INLINE_(set)   ? set_inline_contains_                               \
   : SET_CAN_BITSET_(set) ? set_bitset_contains_                               \
   : SET_CAN_UNBOX_(set)  ? set_unboxed_contains_                              \
                          : set_boxed_contains_)(                               \
      &(set)->impl, (TYPEOF(*(set)->witness)[1]){item}, SET_SIG_(set))

/// get the number of items in a set
///
/// This macro can be thought of as having the C type:
///
///   size_t SET_SIZE(SET(<type> *set);
///
/// @param set Set to operate on
/// @return Size of the set
#define SET_SIZE(set)                                                          \
  (SET_CAN_INLINE_(set)   ? set_inline_size_                                   \
   : SET_CAN_BITSET_(set) ? set_bitset_size_                                   \
   : SET_CAN_UNBOX_(set)  ? set_unboxed_size_                                  \
                          : set_boxed_size_)(&(set)->impl, SET_SIG_(set))

/// clear a set and deallocate its backing resources
///
/// This macro can be thought of as having the C type:
///
///   void SET_FREE(SET(<type>) *set);
///
/// After a call to this function, the set is empty and can be reused.
///
/// @param set Set to operate on
#define SET_FREE(set)                                                          \
  (SET_CAN_INLINE_(set)   ? set_inline_free_                                   \
   : SET_CAN_BITSET_(set) ? set_bitset_free_                                   \
   : SET_CAN_UNBOX_(set)  ? set_unboxed_free_                                  \
                          : set_boxed_free_)(&(set)->impl)

////////////////////////////////////////////////////////////////////////////////
// private API
//
// Everything below this point is not intended to be directly called by
// includers.
////////////////////////////////////////////////////////////////////////////////

/// set private implementation
typedef struct {
  union {
    asp_t root; ///< shared (opaque) pointer to the implementation itself
    _Atomic uintptr_t raw[2]; ///< (opaque) bitset used by inline implementation
  };
} set_t_;

/// the characterisation of a type
typedef struct {
  size_t alignment; ///< required alignment
  size_t size;      ///< byte size

  /// number of values in this type
  ///
  /// This member is only accurate up to `SIZE_MAX >> 8`. A value greater than
  /// this means the count is ≥`(SIZE_MAX >> 8)`.
  size_t count;

  size_t (*hash)(const void *, size_t);           ///< set hasher
  bool (*eq)(const void *, const void *, size_t); ///< set comparator
  void (*dtor)(void *);                           ///< set destructor
} set_sig_t_;

/// is a given value of boolean type?
#define SET_IS_BOOL_(val) _Generic((val), bool: 1, default: 0)

/// construct a `set_sig_t_` from a set type
#define SET_SIG_(set)                                                          \
  ((set_sig_t_){.alignment = alignof(TYPEOF(*(set)->witness)),                 \
                .size = sizeof(*(set)->witness),                               \
                .count = SET_IS_BOOL_(*(set)->witness) ? 2                     \
                         : sizeof(*(set)->witness) < sizeof(size_t)            \
                             ? (size_t)1                                       \
                                   << (sizeof(*(set)->witness) * CHAR_BIT)     \
                             : SIZE_MAX,                                       \
                .hash = sizeof((set)->hash) > 0 ? (set)->hash[0] : NULL,       \
                .eq = sizeof((set)->eq) > 0 ? (set)->eq[0] : NULL,             \
                .dtor = sizeof((set)->dtor) > 0 ? (set)->dtor[0] : NULL})

/// can this set use the optimised inline implementation?
#define SET_CAN_INLINE_(set)                                                   \
  (SET_SIG_(set).count <= sizeof((set)->impl.raw) * CHAR_BIT &&                \
   SET_SIG_(set).hash == NULL && SET_SIG_(set).eq == NULL &&                   \
   SET_SIG_(set).dtor == NULL)

/// can this set use the optimised bitset implementation?
#define SET_CAN_BITSET_(set)                                                   \
  (SET_SIG_(set).size <= 2 && SET_SIG_(set).hash == NULL &&                    \
   SET_SIG_(set).eq == NULL && SET_SIG_(set).dtor == NULL)

/// can this set use the optimised unboxed implementation?
#define SET_CAN_UNBOX_(set)                                                    \
  (SET_SIG_(set).size < sizeof(uintptr_t) &&                                   \
   SET_SIG_(set).alignment <= alignof(uintptr_t) &&                            \
   SET_SIG_(set).dtor == NULL)

////////////////////////////////////////////////////////////////////////////////
// implementations for boxed set
////////////////////////////////////////////////////////////////////////////////

/// insert an item into a boxed set
///
/// @param set Set to operate on
/// @param item Item to insert
/// @param exists [out] If not null, on success this will be set to whether the
///   item already existed in the set before the insertion
/// @param sig Signature of the set item type
/// @return 0 on success or an errno on failure
int set_boxed_insert_(set_t_ *set, void *item, bool *exists, set_sig_t_ sig);

/// remove an item from a boxed set
///
/// @param set Set to operate on
/// @param item Item to remove
/// @param sig Signature of the set item type
/// @return True if the item was previously in the set
bool set_boxed_remove_(set_t_ *set, const void *item, set_sig_t_ sig);

/// check if an item is in a boxed set
///
/// @param set Set to operate on
/// @param item Item to seek
/// @param sig Signature of the set item type
/// @return True if item was found in the set
bool set_boxed_contains_(set_t_ *set, const void *item, set_sig_t_ sig);

/// get the number of items in a boxed set
///
/// @param set Set to operate on
/// @param sig Signature of the set item type
/// @return Size of the set
size_t set_boxed_size_(set_t_ *set, set_sig_t_ sig);

/// clear a boxed set and deallocate its backing resources
///
/// @param set Set to operate on
void set_boxed_free_(set_t_ *set);

////////////////////////////////////////////////////////////////////////////////
// implementations for unboxed set
////////////////////////////////////////////////////////////////////////////////

/// insert an item into an unboxed set
///
/// @param set Set to operate on
/// @param item Item to insert
/// @param exists [out] If not null, on success this will be set to whether the
///   item already existed in the set before the insertion
/// @param sig Signature of the set item type
/// @return 0 on success or an errno on failure
int set_unboxed_insert_(set_t_ *set, void *item, bool *exists, set_sig_t_ sig);

/// remove an item from an unboxed set
///
/// @param set Set to operate on
/// @param item Item to remove
/// @param sig Signature of the set item type
/// @return True if the item was previously in the set
bool set_unboxed_remove_(set_t_ *set, const void *item, set_sig_t_ sig);

/// check if an item is in an unboxed set
///
/// @param set Set to operate on
/// @param item Item to seek
/// @param sig Signature of the set item type
/// @return True if item was found in the set
bool set_unboxed_contains_(set_t_ *set, const void *item, set_sig_t_ sig);

/// get the number of items in an unboxed set
///
/// @param set Set to operate on
/// @param sig Signature of the set item type
/// @return Size of the set
size_t set_unboxed_size_(set_t_ *set, set_sig_t_ sig);

/// clear an unboxed set and deallocate its backing resources
///
/// @param set Set to operate on
void set_unboxed_free_(set_t_ *set);

////////////////////////////////////////////////////////////////////////////////
// implementations for bitset-backed set
////////////////////////////////////////////////////////////////////////////////

/// insert an item into a biset-backed set
///
/// @param set Set to operate on
/// @param item Item to insert
/// @param exists [out] If not null, on success this will be set to whether the
///   item already existed in the set before the insertion
/// @param sig Signature of the set item type
/// @return 0 on success or an errno on failure
int set_bitset_insert_(set_t_ *set, void *item, bool *exists, set_sig_t_ sig);

/// remove an item from a bitset-backed set
///
/// @param set Set to operate on
/// @param item Item to remove
/// @param sig Signature of the set item type
/// @return True if the item was previously in the set
bool set_bitset_remove_(set_t_ *set, const void *item, set_sig_t_ sig);

/// check if an item is in a bitset-backed set
///
/// @param set Set to operate on
/// @param item Item to seek
/// @param sig Signature of the set item type
/// @return True if item was found in the set
bool set_bitset_contains_(set_t_ *set, const void *item, set_sig_t_ sig);

/// get the number of items in a bitset-backed set
///
/// @param set Set to operate on
/// @param sig Signature of the set item type
/// @return Size of the set
size_t set_bitset_size_(set_t_ *set, set_sig_t_ sig);

/// clear a bitset-backed set and deallocate its backing resources
///
/// @param set Set to operate on
void set_bitset_free_(set_t_ *set);

////////////////////////////////////////////////////////////////////////////////
// implementations for inline set
////////////////////////////////////////////////////////////////////////////////

/// insert an item into an inline set
///
/// @param set Set to operate on
/// @param item Item to insert
/// @param exists [out] If not null, on success this will be set to whether the
///   item already existed in the set before the insertion
/// @param sig Signature of the set item type
/// @return 0 on success or an errno on failure
int set_inline_insert_(set_t_ *set, void *item, bool *exists, set_sig_t_ sig);

/// remove an item from an inline set
///
/// @param set Set to operate on
/// @param item Item to remove
/// @param sig Signature of the set item type
/// @return True if the item was previously in the set
bool set_inline_remove_(set_t_ *set, const void *item, set_sig_t_ sig);

/// check if an item is in an inline set
///
/// @param set Set to operate on
/// @param item Item to seek
/// @param sig Signature of the set item type
/// @return True if item was found in the set
bool set_inline_contains_(set_t_ *set, const void *item, set_sig_t_ sig);

/// get the number of items in an inline set
///
/// @param set Set to operate on
/// @param sig Signature of the set item type
/// @return Size of the set
size_t set_inline_size_(set_t_ *set, set_sig_t_ sig);

/// clear an inline set and deallocate its backing resources
///
/// @param set Set to operate on
void set_inline_free_(set_t_ *set);

#ifdef __cplusplus
}
#endif
