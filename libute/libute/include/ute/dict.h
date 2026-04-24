/// @file
/// @brief Type-generic dictionary
///
/// XXX: this implementation is entirely untested as of now
///
/// This dictionary is:
///   • Type-generic – works for any element type
///   • Type-safe – compiler should catch all incorrect parameter passing
///   • Thread-safe – all macros except `DICT_GET` are safe to call concurrently
///   • Lock-free – no mutexes or semaphores involved
///
/// The trade off in being such a general belt-and-suspenders implementation is
/// that it is not particularly fast. But it still aims to be memory efficient.
///
/// See set.h for further discussion of the background of the techniques
/// involved.
///
/// All content in this file is in the public domain. Use it any way you wish.

#pragma once

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <ute/asp.h>
#include <ute/typeof.h>

#ifdef __cplusplus
extern "C" {
#endif

/// a dictionary type mapping keys of a given type to values of a given type
///
/// This expands to a type that is intended to be zero-initialised:
///
///   DICT(int, char) = {0};
///
/// @param key_type Type of keys to the dictionary
/// @param value_type Type of values in the dictionary
#define DICT(key_type, value_type)                                             \
  struct {                                                                     \
    union {                                                                    \
      dict_t_ impl; /**< private implementation */                             \
                                                                               \
      /** mechanism for re-obtaining the dictionary key/value type          */ \
      /*                                                                    */ \
      /* To achieve type safety, we need to be able to refer back to        */ \
      /* `key_type` and `value_type` at call sites involving variables of   */ \
      /* the struct type being defined here. To do this we propagate these  */ \
      /* types through an unused member. This does not need to be a pointer */ \
      /* or be unioned with the `impl` member, but this ensures we minimise */ \
      /* the size of the struct.                                            */ \
      struct {                                                                 \
        key_type k;                                                            \
        value_type v;                                                          \
      } *witness;                                                              \
    };                                                                         \
                                                                               \
    /** optional user-supplied key hash                                     */ \
    /*                                                                      */ \
    /* If this member is not null, it will be called when a key needs to be */ \
    /* hashed.                                                              */ \
    size_t (*hash)(const void *, size_t);                                      \
  }

/// insert or update an entry in a dictionary
///
/// This macro can be thought of as having the C type:
///
///   int DICT_SET(DICT(<key_type>, <value_type>) *dict, const <key_type> key,
///                const <value_type> value);
///
/// @param dict Dictionary to operate on
/// @param key Key to insert
/// @param value Value to insert
/// @return 0 on success or an errno on failure
#define DICT_SET(dict, key, value)                                             \
  dict_set_(&(dict)->impl, (TYPEOF((dict)->witness->k)[1]){key},               \
            (TYPEOF((dict)->witness->v)[1]){value}, DICT_SIG_(dict))

/// retrieve a value from a dictionary
///
/// This macro can be thought of as having the C type:
///
///   <value_type> *DICT_GET(DICT(<key_type>, <value_type>) *dict,
///                          const <key_type> key);
///
/// If the given key is not found in the dictionary, null is returned.
///
/// The pointer returned by this macro is only valid until the next
/// dictionary-modifying operation (`DICT_SET`, `DICT_REMOVE`, `DICT_FREE`).
/// Calling this macro concurrently with any dictionary-modifying operation will
/// result in undefined behaviour.
///
/// @param dict Dictionary to operate on
/// @param key Key to seek
/// @return Pointer to value associated with key or `NULL`
#define DICT_GET(dict, key)                                                    \
  ((TYPEOF(&(dict)->witness->v))dict_get_(                                     \
      &(dict)->impl, (TYPEOF((dict)->witness->k)[1]){key}, DICT_SIG_(dict)))

/// delete an entry from a dictionary
///
/// This macro can be thought of as having the C type:
///
///   bool DICT_REMOVE(DICT(<key_type>, <value_type>) *dict,
///                    const <key_type> key);
///
/// @param dict Dictionary to operate on
/// @param key Key of entry to remove
/// @return True if the entry was found in the dictionary
#define DICT_REMOVE(dict, key)                                                 \
  dict_remove_(&(dict)->impl, (TYPEOF((dict)->witness->k)[1]){key},            \
               DICT_SIG_(dict))

/// does a key exist in a dictionary?
///
/// This macro can be thought of as having the C type:
///
///   bool DICT_CONTAINS(DICT(<key_type>, <value_type>) *dict,
///                      const <key_type> key);
///
/// @param dict Dictionary to operate on
/// @param key Key to seek
/// @return True if the key was found in the dictionary
#define DICT_CONTAINS(dict, key)                                               \
  dict_contains_(&(dict)->impl, (TYPEOF((dict)->witness->k)[1]){key},          \
                 DICT_SIG_(dict))

/// get the number of items in a dictionary
///
/// This macro can be thought of as having the C type:
///
///   size_t DICT_SIZE(DICT(<key_type>, <value_type>) *dict);
///
/// @param dict Dictionary to operate on
/// @return Size of the dictionary
#define DICT_SIZE(dict) dict_size_(&(dict)->impl)

/// clear a dictionary and deallocate its backing resources
///
/// This macro can be thought of as having the C type:
///
///   void DICT_FREE(DICT(<key_type>, <value_type>) *dict);
///
/// After a call to this macro, the dictionary is empty and can be reused.
///
/// @param dict Dictionary to operate on
#define DICT_FREE(dict) dict_free_(&(dict)->impl)

////////////////////////////////////////////////////////////////////////////////
// private API
//
// Everything below this point is not intended to be directly called by
// includers.
////////////////////////////////////////////////////////////////////////////////

/// dictionary private implementation
typedef struct {
  asp_t root; ///< shared pointer to the implementation itself
} dict_t_;

/// the characterisation of a dictionary
typedef struct {
  size_t key_alignment;   ///< required alignment of keys
  size_t key_size;        ///< byte size of keys
  size_t value_alignment; ///< required alignment of values
  size_t value_size;      ///< byte size of values

  size_t (*hash)(const void *, size_t); ///< dictionary hasher
} dict_sig_t_;

/// construct a `dict_sig_t_` from a dictionary type
#define DICT_SIG_(dict)                                                        \
  ((dict_sig_t_){.key_alignment = alignof(TYPEOF((dict)->witness->k)),         \
                 .key_size = sizeof((dict)->witness->k),                       \
                 .value_alignment = alignof(TYPEOF((dict)->witness->v)),       \
                 .value_size = sizeof((dict)->witness->v),                     \
                 .hash = (dict)->hash})

/// insert or update an entry in a dictionary
///
/// @param dict Dictionary to operate on
/// @param key Key to insert
/// @param value Value to insert
/// @param sig Signature of the dictionary
/// @return 0 on success or an errno on failure
int dict_set_(dict_t_ *dict, const void *key, const void *value,
              dict_sig_t_ sig);

/// retrieve a value from a dictionary
///
/// If the given key is not found in the dictionary, null is returned.
///
/// The pointer returned by this function is only valid until the next
/// dictionary-modifying operation (`dict_set_`, `dict_remove_`, `dict_free_`).
/// Calling this function concurrently with any dictionary-modifying operation
/// will result in undefined behaviour.
///
/// @param dict Dictionary to operate on
/// @param key Key to seek
/// @param sig Signature of the dictionary
/// @return Pointer to value associated with key or `NULL`
void *dict_get_(dict_t_ *dict, const void *key, dict_sig_t_ sig);

/// delete an entry from a dictionary
///
/// @param dict Dictionary to operate on
/// @param key Key of entry to remove
/// @param sig Signature of the dictionary
/// @return True if the entry was found in the dictionary
bool dict_remove_(dict_t_ *dict, const void *key, dict_sig_t_ sig);

/// does a key exist in a dictionary?
///
/// @param dict Dictionary to operate on
/// @param key Key to seek
/// @param sig Signature of the dictionary
/// @return True if the key was found in the dictionary
bool dict_contains_(dict_t_ *dict, const void *key, dict_sig_t_ sig);

/// get the number of items in a dictionary
///
/// @param dict Dictionary to operate on
/// @return Size of the dictionary
size_t dict_size_(dict_t_ *dict);

/// clear a dictionary and deallocate its backing resources
///
/// @param dict Dictionary to operate on
void dict_free_(dict_t_ *dict);

#ifdef __cplusplus
}
#endif
