/// @file
/// @brief Test multiple threads using the set.h API
///
/// All content in this file is in the public domain. Use it any way you wish.

#include "test.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <ute/set.h>

typedef SET(unsigned char) chars_t;

typedef struct {
  chars_t *chars;
  size_t thread_id;
  bool remove; ///< remove elements that are added?
} chars_state_t;

/// construct a unique value based on thread ID
static unsigned char make_char_val(int thread_id, int i) {
  return (thread_id << 4) | i;
}

static THREAD_RET char_entry(void *arg) {
  assert(arg != NULL);
  chars_state_t *const s = arg;

  for (int i = 0; i < 10; ++i) {
    const unsigned char v = make_char_val((int)s->thread_id, i);
    const int r = SET_INSERT(s->chars, v);
    ASSERT_EQ(r, 0);

    for (int j = 0; j < 10; ++j) {
      const unsigned char v2 = make_char_val((int)s->thread_id, j);
      const bool present = SET_CONTAINS(s->chars, v2);
      if (j <= i) {
        ASSERT(present);
      } else {
        ASSERT(!present);
      }
    }
  }

  for (int i = 0; s->remove && i < 10; ++i) {
    const unsigned char v = make_char_val((int)s->thread_id, i);
    const bool r = SET_REMOVE(s->chars, v);
    ASSERT(r);

    for (int j = 0; j < 10; ++j) {
      const unsigned char v2 = make_char_val((int)s->thread_id, j);
      const bool present = SET_CONTAINS(s->chars, v2);
      if (j <= i) {
        ASSERT(!present);
      } else {
        ASSERT(present);
      }
    }
  }

  return 0;
}

/// some operations on a char set from multiple threads
TEST("char set multithreaded") {

  chars_t chars = {0};
  thread_t t[16];
  chars_state_t s[sizeof(t) / sizeof(t[0])];

  // setup states
  for (size_t i = 0; i < sizeof(s) / sizeof(s[0]); ++i)
    s[i] = (chars_state_t){.chars = &chars, .thread_id = i, .remove = true};

  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    const int r = THREAD_CREATE(&t[i], char_entry, &s[i]);
    ASSERT_EQ(r, 0);
  }

  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    THREAD_RET ret = 0;
    const int r = THREAD_JOIN(t[i], &ret);
    ASSERT_EQ(r, 0);
    ASSERT_EQ(ret, (THREAD_RET){0});
  }

  ASSERT_EQ(SET_SIZE(&chars), 0u);
  SET_FREE(&chars);
}

/// some operations on a char set from multiple threads, no clean up
TEST("char set multithreaded, leave set non-empty") {

  chars_t chars = {0};
  thread_t t[16];
  chars_state_t s[sizeof(t) / sizeof(t[0])];

  // setup states
  for (size_t i = 0; i < sizeof(s) / sizeof(s[0]); ++i)
    s[i] = (chars_state_t){.chars = &chars, .thread_id = i};

  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    const int r = THREAD_CREATE(&t[i], char_entry, &s[i]);
    ASSERT_EQ(r, 0);
  }

  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    THREAD_RET ret = 0;
    const int r = THREAD_JOIN(t[i], &ret);
    ASSERT_EQ(r, 0);
    ASSERT_EQ(ret, (THREAD_RET){0});
  }

  SET_FREE(&chars);
}

typedef SET(unsigned short) shorts_t;

typedef struct {
  shorts_t *shorts;
  size_t thread_id;
  bool remove; ///< remove elements that are added?
} shorts_state_t;

/// construct a unique value based on thread ID
static unsigned short make_short_val(int thread_id, int i) {
  return (thread_id << 8) | i;
}

static THREAD_RET short_entry(void *arg) {
  assert(arg != NULL);
  shorts_state_t *const s = arg;

  for (int i = 0; i < 10; ++i) {
    const unsigned short v = make_short_val((int)s->thread_id, i);
    const int r = SET_INSERT(s->shorts, v);
    ASSERT_EQ(r, 0);

    for (int j = 0; j < 10; ++j) {
      const unsigned short v2 = make_short_val((int)s->thread_id, j);
      const bool present = SET_CONTAINS(s->shorts, v2);
      if (j <= i) {
        ASSERT(present);
      } else {
        ASSERT(!present);
      }
    }
  }

  for (int i = 0; s->remove && i < 10; ++i) {
    const unsigned short v = make_short_val((int)s->thread_id, i);
    const bool r = SET_REMOVE(s->shorts, v);
    ASSERT(r);

    for (int j = 0; j < 10; ++j) {
      const unsigned short v2 = make_short_val((int)s->thread_id, j);
      const bool present = SET_CONTAINS(s->shorts, v2);
      if (j <= i) {
        ASSERT(!present);
      } else {
        ASSERT(present);
      }
    }
  }

  return 0;
}

/// some operations on a short set from multiple threads
TEST("short set multithreaded") {

  shorts_t shorts = {0};
  thread_t t[32];
  shorts_state_t s[sizeof(t) / sizeof(t[0])];

  // setup states
  for (size_t i = 0; i < sizeof(s) / sizeof(s[0]); ++i)
    s[i] = (shorts_state_t){.shorts = &shorts, .thread_id = i, .remove = true};

  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    const int r = THREAD_CREATE(&t[i], short_entry, &s[i]);
    ASSERT_EQ(r, 0);
  }

  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    THREAD_RET ret = 0;
    const int r = THREAD_JOIN(t[i], &ret);
    ASSERT_EQ(r, 0);
    ASSERT_EQ(ret, (THREAD_RET){0});
  }

  ASSERT_EQ(SET_SIZE(&shorts), 0u);
  SET_FREE(&shorts);
}

/// some operations on a short set from multiple threads, no clean up
TEST("short set multithreaded, leave set non-empty") {

  shorts_t shorts = {0};
  thread_t t[32];
  shorts_state_t s[sizeof(t) / sizeof(t[0])];

  // setup states
  for (size_t i = 0; i < sizeof(s) / sizeof(s[0]); ++i)
    s[i] = (shorts_state_t){.shorts = &shorts, .thread_id = i};

  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    const int r = THREAD_CREATE(&t[i], short_entry, &s[i]);
    ASSERT_EQ(r, 0);
  }

  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    THREAD_RET ret = 0;
    const int r = THREAD_JOIN(t[i], &ret);
    ASSERT_EQ(r, 0);
    ASSERT_EQ(ret, (THREAD_RET){0});
  }

  SET_FREE(&shorts);
}

typedef SET(int) ints_t;

typedef struct {
  ints_t *ints;
  size_t thread_id;
  bool remove; ///< remove elements that are added?
} ints_state_t;

/// construct a unique value based on thread ID
static int make_int_val(int thread_id, int i) { return (thread_id << 8) | i; }

static THREAD_RET int_entry(void *arg) {
  assert(arg != NULL);
  ints_state_t *const s = arg;

  for (int i = 0; i < 10; ++i) {
    const int v = make_int_val((int)s->thread_id, i);
    const int r = SET_INSERT(s->ints, v);
    ASSERT_EQ(r, 0);

    for (int j = 0; j < 10; ++j) {
      const int v2 = make_int_val((int)s->thread_id, j);
      const bool present = SET_CONTAINS(s->ints, v2);
      if (j <= i) {
        ASSERT(present);
      } else {
        ASSERT(!present);
      }
    }
  }

  for (int i = 0; s->remove && i < 10; ++i) {
    const int v = make_int_val((int)s->thread_id, i);
    const bool r = SET_REMOVE(s->ints, v);
    ASSERT(r);

    for (int j = 0; j < 10; ++j) {
      const int v2 = make_int_val((int)s->thread_id, j);
      const bool present = SET_CONTAINS(s->ints, v2);
      if (j <= i) {
        ASSERT(!present);
      } else {
        ASSERT(present);
      }
    }
  }

  return 0;
}

/// some operations on an integer set from multiple threads
TEST("int set multithreaded") {

  ints_t ints = {0};
  thread_t t[32];
  ints_state_t s[sizeof(t) / sizeof(t[0])];

  // setup states
  for (size_t i = 0; i < sizeof(s) / sizeof(s[0]); ++i)
    s[i] = (ints_state_t){.ints = &ints, .thread_id = i, .remove = true};

  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    const int r = THREAD_CREATE(&t[i], int_entry, &s[i]);
    ASSERT_EQ(r, 0);
  }

  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    THREAD_RET ret = 0;
    const int r = THREAD_JOIN(t[i], &ret);
    ASSERT_EQ(r, 0);
    ASSERT_EQ(ret, (THREAD_RET){0});
  }

  ASSERT_EQ(SET_SIZE(&ints), 0u);
  SET_FREE(&ints);
}

/// some operations on an integer set from multiple threads, no clean up
TEST("int set multithreaded, leave set non-empty") {

  ints_t ints = {0};
  thread_t t[32];
  ints_state_t s[sizeof(t) / sizeof(t[0])];

  // setup states
  for (size_t i = 0; i < sizeof(s) / sizeof(s[0]); ++i)
    s[i] = (ints_state_t){.ints = &ints, .thread_id = i};

  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    const int r = THREAD_CREATE(&t[i], int_entry, &s[i]);
    ASSERT_EQ(r, 0);
  }

  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    THREAD_RET ret = 0;
    const int r = THREAD_JOIN(t[i], &ret);
    ASSERT_EQ(r, 0);
    ASSERT_EQ(ret, (THREAD_RET){0});
  }

  SET_FREE(&ints);
}

typedef SET(unsigned long) longs_t;

typedef struct {
  longs_t *longs;
  size_t thread_id;
  bool remove; ///< remove elements that are added?
} longs_state_t;

/// construct a unique value based on thread ID
static unsigned long make_long_val(int thread_id, int i) {
  return (thread_id << 8) | i;
}

static THREAD_RET long_entry(void *arg) {
  assert(arg != NULL);
  longs_state_t *const s = arg;

  for (int i = 0; i < 10; ++i) {
    const unsigned long v = make_long_val((int)s->thread_id, i);
    const int r = SET_INSERT(s->longs, v);
    ASSERT_EQ(r, 0);

    for (int j = 0; j < 10; ++j) {
      const unsigned long v2 = make_long_val((int)s->thread_id, j);
      const bool present = SET_CONTAINS(s->longs, v2);
      if (j <= i) {
        ASSERT(present);
      } else {
        ASSERT(!present);
      }
    }
  }

  for (int i = 0; s->remove && i < 10; ++i) {
    const unsigned long v = make_long_val((int)s->thread_id, i);
    const bool r = SET_REMOVE(s->longs, v);
    ASSERT(r);

    for (int j = 0; j < 10; ++j) {
      const unsigned long v2 = make_long_val((int)s->thread_id, j);
      const bool present = SET_CONTAINS(s->longs, v2);
      if (j <= i) {
        ASSERT(!present);
      } else {
        ASSERT(present);
      }
    }
  }

  return 0;
}

/// some operations on a long set from multiple threads
TEST("long set multithreaded") {

  longs_t longs = {0};
  thread_t t[32];
  longs_state_t s[sizeof(t) / sizeof(t[0])];

  // setup states
  for (size_t i = 0; i < sizeof(s) / sizeof(s[0]); ++i)
    s[i] = (longs_state_t){.longs = &longs, .thread_id = i, .remove = true};

  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    const int r = THREAD_CREATE(&t[i], long_entry, &s[i]);
    ASSERT_EQ(r, 0);
  }

  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    THREAD_RET ret = 0;
    const int r = THREAD_JOIN(t[i], &ret);
    ASSERT_EQ(r, 0);
    ASSERT_EQ(ret, (THREAD_RET){0});
  }

  ASSERT_EQ(SET_SIZE(&longs), 0u);
  SET_FREE(&longs);
}

/// some operations on a long set from multiple threads, no clean up
TEST("long set multithreaded, leave set non-empty") {

  longs_t longs = {0};
  thread_t t[32];
  longs_state_t s[sizeof(t) / sizeof(t[0])];

  // setup states
  for (size_t i = 0; i < sizeof(s) / sizeof(s[0]); ++i)
    s[i] = (longs_state_t){.longs = &longs, .thread_id = i};

  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    const int r = THREAD_CREATE(&t[i], long_entry, &s[i]);
    ASSERT_EQ(r, 0);
  }

  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    THREAD_RET ret = 0;
    const int r = THREAD_JOIN(t[i], &ret);
    ASSERT_EQ(r, 0);
    ASSERT_EQ(ret, (THREAD_RET){0});
  }

  SET_FREE(&longs);
}
