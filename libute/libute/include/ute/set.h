/// @file
/// @brief Type-generic set
///
/// This set is:
///   • Type-generic – works for any element type (WIP)
///   • Type-safe – compiler should catch all incorrect parameter passing
///   • Thread-safe – all macros are safe to call concurrently
///   • Lock-free – no mutexes or semaphores involved
///
/// The trade off in being such a general belt-and-suspenders implementation is
/// that it is not particularly fast. But it still aims to be memory efficient.
///
/// The type-generic, type-safe aspect of this is based on techniques from:
///   Type Safe Generic Data Structures in C
///   Daniel Hooper
///   https://danielchasehooper.com/posts/typechecked-generic-c-data-structures
///
/// The thread-safe aspect of this is based on techniques from:
///   A Lock-Free Wait-Free Hash Table
///   Dr Cliff Click
///   https://web.stanford.edu/class/ee380/Abstracts/070221_LockFreeHash.pdf
///
/// TODO:
///   • custom hash
///   • custom eq
///   • dtor
///   • support char *
///   • unboxing optimisation
///   • return “inserted?” indication in SET_INSERT
///
/// All content in this file is in the public domain. Use it any way you wish.

#pragma once

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
/// @param type Type of items that will be stored in the set
#define SET(type)                                                              \
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
                                                                               \
      /** mechanism for re-obtaining the alignment of the set item type     */ \
      /*                                                                    */ \
      /* One might think this could already be recovered by                 */ \
      /* `alignof(*witness)`. But calling `alignof` on an expression is a   */ \
      /* GNU extension not supported by Clang. Using a VLA in a struct is   */ \
      /* an extension too, but one supported by both Clang and GCC.         */ \
      char (*alignment)[alignof(type)];                                        \
    } u_;                                                                      \
  }

/// insert an item into a set
///
/// This macro can be thought of as having the C type:
///
///   int SET_INSERT(SET(<type>) *set, const <type> item);
///
/// @param set Set to operate on
/// @param item Item to insert
/// @return 0 on success or an errno on failure
#define SET_INSERT(set, item)                                                  \
  set_insert_(&(set)->u_.impl, (TYPEOF(*(set)->u_.witness)[1]){item},          \
              sizeof(*(set)->u_.alignment), sizeof(*(set)->u_.witness))

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
  set_remove_(&(set)->u_.impl, (TYPEOF(*(set)->u_.witness)[1]){item},          \
              sizeof(*(set)->u_.witness))

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
  set_contains_(&(set)->u_.impl, (TYPEOF(*(set)->u_.witness)[1]){item},        \
                sizeof(*(set)->u_.witness))

/// get the number of items in a set
///
/// This macro can be thought of as having the C type:
///
///   size_t SET_SIZE(SET(<type> *set);
///
/// @param set Set to operate on
/// @return Size of the set
#define SET_SIZE(set) set_size_(&(set)->u_.impl)

/// clear a set and deallocate its backing resources
///
/// This macro can be thought of as having the C type:
///
///   void SET_FREE(SET(<type>) *set);
///
/// After a call to this function, the set is empty and can be reused.
///
/// @param set Set to operate on
#define SET_FREE(set) set_free_(&(set)->u_.impl)

////////////////////////////////////////////////////////////////////////////////
// private API
//
// Everything below this point is not intended to be directly called by
// includers.
////////////////////////////////////////////////////////////////////////////////

/// set private implementation
typedef struct {
  asp_t root; ///< shared (opaque) pointer to the implementation itself
} set_t_;

/// insert an item into a set
///
/// @param set Set to operate on
/// @param item Item to insert
/// @param item_alignment Byte alignment of `item`
/// @param item_size Byte size of `item`
/// @return 0 on success or an errno on failure
int set_insert_(set_t_ *set, const void *item, size_t item_alignment,
                size_t item_size);

/// remove an item from a set
///
/// @param set Set to operate on
/// @param item Item to remove
/// @param item_size Byte size of `item`
/// @return True if the item was previously in the set
bool set_remove_(set_t_ *set, const void *item, size_t item_size);

/// check if an item is in a set
///
/// @param set Set to operate on
/// @param item Item to seek
/// @param item_size Byte size of `item`
/// @return True if item was found in the set
bool set_contains_(set_t_ *set, const void *item, size_t item_size);

/// get the number of items in a set
///
/// @param set Set to operate on
/// @return Size of the set
size_t set_size_(set_t_ *set);

/// clear a set and deallocate its backing resources
///
/// @param set Set to operate on
void set_free_(set_t_ *set);

#ifdef __cplusplus
}
#endif
