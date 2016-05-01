/* An experiment in implementing RAII in C.
 *
 * This is mostly just a thought experiment and it's fairly suboptimal. In particular, I think
 * building a linked-list of destructors is too complicated for the compiler to see through. In my
 * experience, GCC is capable of inlining a `cleanup` function, even when it closes over a local
 * variable. The advantage of this is that "raw" RAII implemented with explicit `cleanup`
 * attributes ends up being no less efficient than manually tracking and spelling out cleanup
 * operations. However, building a linked-list like this impedes the compiler's optimiser enough
 * that it cannot collapse/inline the cleanup operations.
 */

#define _XOPEN_SOURCE 700
#include <stdlib.h>
#include <string.h>

#define _JOIN(x, y) x ## y
#define JOIN(x, y) _JOIN(x, y)

#define RAII_INIT() \
    struct _raii_data { \
        void (*function)(void); \
        int when; \
        struct _raii_data *next; \
    }; \
    void _raii_cleanup(void *p) { \
        struct _raii_data *r = *(struct _raii_data**)p; \
        while (r != NULL) { \
            if (r->when == 0) { \
                r->function(); \
            } \
            r = r->next; \
        } \
    } \
    struct _raii_data *_raii __attribute__((cleanup(_raii_cleanup))) = NULL

#define RAII_PREPEND(wh) \
    auto void JOIN(_raii_, __LINE__)(void); \
    struct _raii_data JOIN(_raii_data_, __LINE__) = { \
        .function = JOIN(_raii_, __LINE__), \
        .when = wh, \
        .next = _raii, \
    }; \
    _raii = &JOIN(_raii_data_, __LINE__); \
    void JOIN(_raii_, __LINE__)(void)

#define RAII_ON_EXIT()    RAII_PREPEND(0)
#define RAII_ON_SUCCESS() RAII_PREPEND(1)
#define RAII_ON_FAILURE() RAII_PREPEND(2)

#define RAII_CLEAR(wh) \
    do { \
        while (_raii != NULL) { \
            if (_raii->when == wh || _raii->when == 0) { \
                _raii->function(); \
            } \
            _raii = _raii->next; \
        } \
    } while (0)

#define RAII_SUCCESS() RAII_CLEAR(1)
#define RAII_FAILURE() RAII_CLEAR(2)

int foo(char **s) {

    RAII_INIT();

    char *temp = strdup("hello");
    RAII_ON_EXIT() {
        if (temp != NULL)
            free(temp);
    }

    if (temp == NULL) {
        RAII_FAILURE();
        return -1;
    }

    if (s == NULL) {
        RAII_FAILURE();
        return -1;
    }

    *s = strdup(temp);
    RAII_ON_FAILURE() {
        if (*s != NULL)
            free(*s);
    }

    RAII_SUCCESS();
    return 0;
}
