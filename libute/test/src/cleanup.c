/// @file
/// @brief Test cleanup functionality for libute
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <stdlib.h>

cleanup_t *cleanups;

void register_cleanup(void (*function)(void *arg), void *arg) {
  cleanup_t *const new = malloc(sizeof(*new));
  ASSERT_NOT_NULL(new);

  new->function = function;
  new->arg = arg;
  new->next = cleanups;
  cleanups = new;
}

void run_cleanups(void) {
  while (cleanups != NULL) {
    cleanups->function(cleanups->arg);
    cleanup_t *const c = cleanups->next;
    free(cleanups);
    cleanups = c;
  }
}
