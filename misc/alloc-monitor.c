#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <dlfcn.h>
#include <assert.h>

/* Catch one of the gotchas. */
#if !defined(__pic__) && !defined(__PIC__)
    #warning "It looks like you forgot to pass -fPIC."
#endif

/* Location of your libc. You may need to alter this. */
#define LIBC "libc.so.6"

/* In case of unrecoverable exception. */
#define DIE(...) \
    do { \
        fprintf(stderr, __VA_ARGS__); \
        panic = 1; \
        exit(1); \
    } while(0)

/* Helper macros. */
#define LOAD_FN(fn_ptr, lib, symbol) \
    do { \
        (fn_ptr) = dlsym((lib), #symbol); \
        if (!(fn_ptr)) \
        DIE("Alloc Monitor: Failed to load %s from %s: %s\n", #symbol, #lib, \
            dlerror()); \
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
static void *libc;
static void *(*real_malloc)(size_t);
static void  (*real_free)(void*);
static void *(*real_realloc)(void*, size_t);

void alloc_monitor_init(void) __attribute__((constructor));
void alloc_monitor_init(void) {
    if (pthread_mutex_init(&heap_pointers_lock, NULL))
        DIE("Alloc Monitor: Failed to initialise heap_pointers_lock.\n");

    if (!(libc = dlopen(LIBC, RTLD_LAZY)))
        DIE("Alloc Monitor: Failed to load libc.\n");
    LOAD_FN(real_malloc, libc, malloc);
    LOAD_FN(real_free, libc, free);
    LOAD_FN(real_realloc, libc, realloc);
}

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
    if (dlclose(libc))
        DIE("Alloc Monitor: Failed to unload libc when unloading.\n");
}

static int exists(void *ptr) {
    struct list *l;

    LOCK(heap_pointers_lock);

    for (l = heap_pointers; l; l = l->next)
        if (ptr == l->value)
            break;
    
    UNLOCK(heap_pointers_lock);

    return !!l;
}

static void add(void *ptr) {
    struct list *l;

    LOCK(heap_pointers_lock);
    l = (struct list*)real_malloc(sizeof(struct list));
    l->value = ptr;
    l->next = heap_pointers;
    heap_pointers = l;
    UNLOCK(heap_pointers_lock);
}

static void delete(void *ptr) {
    struct list *l, *temp;

    assert(exists(ptr));
    LOCK(heap_pointers_lock);
    for (l = heap_pointers, temp = NULL; l; temp = l, l = l->next) {
        if (l->value == ptr) {
            /* We've found the element we're after. */
            if (temp) temp->next = l->next;
            else      heap_pointers = l->next;
            real_free(l);
            UNLOCK(heap_pointers_lock);
            return;
        }
    }
    /* We should never reach this point. */
    DIE("Alloc Monitor: Failed to find pointer while attempting to delete "
        "(framework failure?).\n");
}

void *malloc(size_t size) {
    void *ptr;

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
    real_free(ptr);
}

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

    ptr2 = real_realloc(ptr, size);
    if (!ptr2)
        fprintf(stderr, "Alloc Monitor: Warning: returning null pointer from "
                "realloc.\n");
    else
        add(ptr2);
    return ptr2;
}
