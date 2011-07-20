/* Tool for instrumenting memory allocation functions in C.
 *
 * C programmers are traditionally lax at handling failure in a call to malloc
 * or one of its cousins. The reasons behind this range from "there is no good
 * way to handle malloc failing" to "if malloc fails we're likely to be
 * murdered by the OOMK imminently" to just plain laziness. I initially wrote
 * this code to probe the effects of malloc failures deep within a third-party
 * tool, but have since found it useful as a base template for instrumenting
 * allocation in my own code.
 *
 * To compile:
 *  gcc -shared -lpthread -ldl -fPIC -o alloc-monitor.so alloc-monitor.c
 * To run:
 *  LD_PRELOAD=./alloc-monitor.so your-program
 *
 * You'll probably want to modify the source below to do something useful for
 * your purposes. The code does nothing currently other than wrap allocation
 * functions and provide warnings when they are called in questionable ways.
 * There are some suggestions for modifications below in malloc.
 *
 * Note that there are two nice ways of dynamically resolving a pointer to a
 * library function: (1) call dlopen and then dlsym with the handle you
 * received or (2) call dlsym with RTLD_NEXT. The second method has its
 * pitfalls in that RTLD_NEXT is a GCC-specific symbol and it's a (slightly)
 * more dangerous way of loading a symbol. Because of the way an RTLD_NEXT
 * dlsym call works, you could actually end up with a pointer to an unexpected
 * function if the system you are executing on has an unexpected library setup.
 * Unfortunately in this case we have no choice because dlopen uses malloc so
 * things go horribly wrong if we use technique (1).
 */
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <dlfcn.h>
#include <assert.h>

/* Catch one of the gotchas. */
#if !defined(__pic__) && !defined(__PIC__)
    #warning "It looks like you forgot to pass -fPIC."
#endif

/* In case of an unrecoverable failure. */
#define DIE(...) \
    do { \
        fprintf(stderr, __VA_ARGS__); \
        panic = 1; \
        exit(1); \
    } while(0)

/* Helper macros. */
#define LOAD_FN(fn_ptr, symbol) \
    do { \
        assert(!(fn_ptr)); \
        (fn_ptr) = dlsym(RTLD_NEXT, #symbol); \
        if (!(fn_ptr)) \
        DIE("Alloc Monitor: Failed to load %s: %s\n", #symbol, dlerror()); \
    } while (0)

#define LOCK(l) \
    do { \
        if (pthread_mutex_lock(&(l))) \
            DIE("Alloc Monitor: Failed to lock %s in %s.\n", #l, \
                __FUNCTION__); \
    } while(0)

#define UNLOCK(l) \
    do { \
        if (pthread_mutex_unlock(&(l))) \
            DIE("Alloc Monitor: Failed to unlock %s in %s.\n", #l, \
                __FUNCTION__); \
    } while(0)

/* Basic linked-list structure. */
struct list {
    const void *value;
    struct list *next;
};

/* List of pointers currently allocated. */
static struct list *heap_pointers;
static pthread_mutex_t heap_pointers_lock;

/* Whether we've called DIE or not. */
static int panic;

/* Function pointers to the real memory functions. */
static void *(*real_malloc)(size_t);
static void  (*real_free)(void*);
static void *(*real_realloc)(void*, size_t);

/* Setup relevant resources for this library. The GCC constructor trick ensures
 * this function is called before anything else in this library.
 */
void alloc_monitor_init(void) __attribute__((constructor));
void alloc_monitor_init(void) {
    assert(!heap_pointers);
    if (pthread_mutex_init(&heap_pointers_lock, NULL))
        DIE("Alloc Monitor: Failed to initialise heap_pointers_lock.\n");

    LOAD_FN(real_malloc, malloc);
    LOAD_FN(real_free, free);
    LOAD_FN(real_realloc, realloc);
}

/* Destroy relevant resources for this library. The GCC destructor trick
 * ensures this function is called just before the library is unloaded.
 */
void alloc_monitor_destroy(void) __attribute__((destructor));
void alloc_monitor_destroy(void) {
    struct list *temp;

    assert(real_free);
    while (heap_pointers) {
        /* We can trash the pointer heap_pointers because we're exiting. */
        temp = heap_pointers;
        heap_pointers = heap_pointers->next;
        if (!panic)
            fprintf(stderr, "Alloc Monitor: Warning: exiting without freeing "
                    "%p.\n", temp->value);
        real_free(temp);
    }
    heap_pointers = NULL;

    if (pthread_mutex_destroy(&heap_pointers_lock))
        DIE("Alloc Monitor: Failed to destroy heap_pointers_lock when "
            "unloading.\n");

    real_malloc = NULL;
    real_free = NULL;
    real_realloc = NULL;
}

/* Returns 1 if this lock is in the current list of allocated pointers and 0
 * otherwise.
 */
static int exists(void *ptr) {
    struct list *l;

    LOCK(heap_pointers_lock);
    for (l = heap_pointers; l; l = l->next)
        if (ptr == l->value)
            break;
    UNLOCK(heap_pointers_lock);

    return !!l;
}

/* Adds a pointer to the list of allocated pointers. */
static void add(void *ptr) {
    struct list *l;

    LOCK(heap_pointers_lock);
    l = (struct list*)real_malloc(sizeof(struct list));
    if (!l)
        DIE("Alloc Monitor: Failed to allocate memory for new internal "
            "resource.\n");
    l->value = ptr;
    l->next = heap_pointers;
    heap_pointers = l;
    UNLOCK(heap_pointers_lock);
}

/* Remove a pointer from the list of allocated pointers. */
static void delete(void *ptr) {
    struct list *l, *temp;

    assert(exists(ptr));
    LOCK(heap_pointers_lock);
    for (l = heap_pointers, temp = NULL; l; temp = l, l = l->next) {
        if (l->value == ptr) {
            /* We've found the element we're after. */
            if (temp) temp->next = l->next;
            else      heap_pointers = l->next;
            assert(real_free);
            real_free(l);
            UNLOCK(heap_pointers_lock);
            return;
        }
    }
    /* We should never reach this point. */
    DIE("Alloc Monitor: Failed to find pointer while attempting to delete "
        "(framework failure?).\n");
}

/* Wrapper functions. */

void *malloc(size_t size) {
    void *ptr;

    /* Here's where you might want to add some interesting behaviour. Typically
     * you'll want to return NULL in certain cases to simulate a failure of
     * malloc. How you decide which calls fail is up to you. Some examples:
     *  (1) Random failure. Not particularly useful because failures are not
     *      easily reproducible.
     *       if (rand()%2) return 0;
     *  (2) Fail after a certain number of mallocs. Requires implementing a
     *      function to determine the size of heap_pointers.
     *       if (len() > x) return 0;
     *  (3) Fail on a certain call to malloc (my personal favourite ;). First
     *      disassemble the target program (objdump -D your-program | less).
     *      Find the instruction following the call you're interested in and
     *      note its address. Now use a stack smash combined with a check for
     *      this address. Note you may have to juggle the value 4 for your
     *      compiler and optimisation level.
     *       if (*(&ptr + 4) == 0x8048354a) return 0;
     *
     * If you're adding bits and pieces here, remember that you may want to add
     * the same logic to realloc and calloc.
     */

    assert(real_malloc);
    ptr = real_malloc(size);
    if (ptr)
        add(ptr);
    else
        fprintf(stderr, "Alloc Monitor: Warning: returning null pointer from "
                "malloc.\n");
    return ptr;
}

void free(void *ptr) {
    if (!ptr)
        fprintf(stderr, "Alloc Monitor: Warning: attempt to free null "
                "pointer.\n");
    else if (!exists(ptr))
        fprintf(stderr, "Alloc Monitor: Warning: attempt to free a pointer "
                "that does not appear to have been allocated (double "
                "free?).\n");
    else
        delete(ptr);
    assert(real_free);
    real_free(ptr);
}

/* Note the various conditions in the realloc wrapper to catch "abuses" of
 * realloc. By this I mean using realloc to malloc or free, which, while legal,
 * is a quick way to make your code untraceable. You may want to remove the
 * warnings if you really think you know what you're doing.
 */
void *realloc(void *ptr, size_t size) {
    void *ptr2;

    if (!exists(ptr))
        /* Code using realloc instead of malloc. */
        fprintf(stderr, "Alloc Monitor: Warning: attempt to realloc a pointer "
                "that does not appear to have been allocated (realloc "
                "abuse?).\n");
    else
        delete(ptr);

    if (!size)
        fprintf(stderr, "Alloc Monitor: Warning: attempt to free through "
                "realloc (realloc abuse?).\n");

    assert(real_realloc);
    ptr2 = real_realloc(ptr, size);
    if (!ptr2)
        fprintf(stderr, "Alloc Monitor: Warning: returning null pointer from "
                "realloc.\n");
    else
        add(ptr2);
    return ptr2;
}
