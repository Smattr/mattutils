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
///   • custom hash
///   • custom eq
///   • support char *
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
                                                                               \
    /** optional user-supplied item destructor                              */ \
    /*                                                                      */ \
    /* If this member is non-null, it will be called on set items           */ \
    /* immediately before they are removed from the set.                    */ \
    void (*dtor)(void *);                                                      \
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
  (SET_CAN_BITSET_(set)  ? set_bitset_insert_                                  \
   : SET_CAN_UNBOX_(set) ? set_unboxed_insert_                                 \
                         : set_boxed_insert_)(                                 \
      &(set)->u_.impl, (TYPEOF(*(set)->u_.witness)[1]){item}, SET_SIG_(set),   \
      (set)->dtor)

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
  (SET_CAN_BITSET_(set)  ? set_bitset_remove_                                  \
   : SET_CAN_UNBOX_(set) ? set_unboxed_remove_                                 \
                         : set_boxed_remove_)(                                 \
      &(set)->u_.impl, (TYPEOF(*(set)->u_.witness)[1]){item}, SET_SIG_(set))

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
  (SET_CAN_BITSET_(set)  ? set_bitset_contains_                                \
   : SET_CAN_UNBOX_(set) ? set_unboxed_contains_                               \
                         : set_boxed_contains_)(                               \
      &(set)->u_.impl, (TYPEOF(*(set)->u_.witness)[1]){item}, SET_SIG_(set))

/// get the number of items in a set
///
/// This macro can be thought of as having the C type:
///
///   size_t SET_SIZE(SET(<type> *set);
///
/// @param set Set to operate on
/// @return Size of the set
#define SET_SIZE(set)                                                          \
  (SET_CAN_BITSET_(set)  ? set_bitset_size_                                    \
   : SET_CAN_UNBOX_(set) ? set_unboxed_size_                                   \
                         : set_boxed_size_)(&(set)->u_.impl, SET_SIG_(set))

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
  (SET_CAN_BITSET_(set)  ? set_bitset_free_                                    \
   : SET_CAN_UNBOX_(set) ? set_unboxed_free_                                   \
                         : set_boxed_free_)(&(set)->u_.impl)

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

/// the characterisation of a type
typedef struct {
  size_t alignment;     ///< required alignment
  size_t size;          ///< byte size
  void (*dtor)(void *); ///< set destructor
} set_sig_t_;

/// construct a `set_sig_t_` from a set type
#define SET_SIG_(set)                                                          \
  ((set_sig_t_){.alignment = sizeof(*(set)->u_.alignment),                     \
                .size = sizeof(*(set)->u_.witness),                            \
                .dtor = (set)->dtor})

/// can this set use the optimised bitset implementation?
#define SET_CAN_BITSET_(set)                                                   \
  (SET_SIG_(set).size <= 2 && SET_SIG_(set).dtor == NULL)

/// can this set use the optimised unboxed implementation?
#define SET_CAN_UNBOX_(set)                                                    \
  (SET_SIG_(set).size < sizeof(uintptr_t) &&                                   \
   SET_SIG_(set).alignment <= alignof(uintptr_t))

////////////////////////////////////////////////////////////////////////////////
// implementations for boxed set
////////////////////////////////////////////////////////////////////////////////

/// insert an item into a boxed set
///
/// @param set Set to operate on
/// @param item Item to insert
/// @param sig Signature of the set item type
/// @param user_dtor User-supplied destructor
/// @return 0 on success or an errno on failure
int set_boxed_insert_(set_t_ *set, const void *item, set_sig_t_ sig,
                      void (*user_dtor)(void *));

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
/// @param sig Signature of the set item type
/// @param user_dtor User-supplied destructor
/// @return 0 on success or an errno on failure
int set_unboxed_insert_(set_t_ *set, const void *item, set_sig_t_ sig,
                        void (*user_dtor)(void *));

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
/// @param sig Signature of the set item type
/// @param user_dtor User-supplied destructor
/// @return 0 on success or an errno on failure
int set_bitset_insert_(set_t_ *set, const void *item, set_sig_t_ sig,
                       void (*user_dtor)(void *));

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

#ifdef __cplusplus
}
#endif
