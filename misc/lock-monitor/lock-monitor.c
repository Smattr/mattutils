/* A utility for monitoring locking behaviour within a C program.
 *
 * One of the common strategies for avoiding deadlocks and livelocks within a
 * concurrent program is developing a partial ordering or hierarchy of the
 * locks within your code. For example, if you have two locks, A and B, making
 * a rule that threads always grab lock A before grabbing lock B will help
 * prevent deadlocks/livelocks. Unfortunately you are sometimes called upon to
 * maintain a codebase that has not been written with these considerations in
 * mind (or even worse, you yourself have made significant headway in a project
 * without taking these factors into account). The locking behaviour of this
 * code often has very poorly defined semantics and sometimes you just don't
 * have time (or can't be bothered) divining those semantics.
 *
 * This utility allows you to instrument a pre-compiled binary to monitor its
 * locking behaviour and detect violations of a perceived partial ordering. If
 * you are using the utility it is probably wise to comprehensively read
 * through the code below to understand exactly what it is checking for. Note
 * that the instrumented functions only relate to semaphores (sem_t) and
 * mutexes (pthread_mutex_t). If you are using some other method of locking you
 * may need to modify the code.
 *
 * To compile:
 *  gcc -shared -lpthread -ldl -fPIC -o lock-monitor.so lock-monitor.c
 * To run:
 *  LD_PRELOAD=./lock-monitor.so your-program
 *
 * Read the included example.c if this does not explain everything. Note that
 * this instrumentation will significantly slow down locking/unlocking
 * operations and these are typically on the critical path of your application.
 * Do not use this on a production build unless you are prepared for things to
 * slow down. Significantly.
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
#include <semaphore.h>
#include <dlfcn.h>
#include <assert.h>

/* Catch one of the gotchas. */
#if !defined(__pic__) && !defined(__PIC__)
    #warning "It looks like you forgot to pass -fPIC."
#endif

/* Name of your platform's shared pthreads library. Depending on your
 * distribution you may need to alter this line. If you're having trouble the
 * output of the following command will probably give you some hints:
 *  find /lib -name "*pthread*"
 */
#define LIBPTHREAD "libpthread.so.0"

/* Some helper macros. */
#define LOAD_FN(fn_ptr, lib, symbol) \
    do { \
        (fn_ptr) = dlsym((lib), #symbol); \
        if (!(fn_ptr)) \
            lock_monitor_exception("Failed to load %s from %s: %s\n", \
                #symbol, #lib, dlerror()); \
            /* Unreachable. */ \
    } while (0)

#define INIT_LOCK(lock) \
    do { \
        if (pthread_mutex_init(&(lock), NULL)) \
            lock_monitor_exception("Lock Monitor: Failed to initialise %s " \
                "lock.\n", #lock); \
            /* Unreachable. */ \
    } while (0)

#define DESTROY_LOCK(lock) \
    do { \
        if (pthread_mutex_destroy(&(lock))) \
            lock_monitor_exception("Lock Monitor: Failed to destroy %s " \
                "lock.\n", #lock); \
            /* Unreachable. */ \
    } while (0)

/* A basic linked-list node. */
struct list {
    const void *value;
    struct list *next;
};

/* Structure for encapsulating information about a thread. */
struct thread {
    pthread_t thread;
    struct list *locks; /* Locks currently held by this thread. */
};

/* A list of threads in the system. This list is extended as threads call
 * locking functions and are thus discovered. Note that new threads are added
 * to the front of the list. This will become important later on.
 */
static struct list *thread_list;
static pthread_mutex_t thread_list_lock;

/* A partial ordering for the locks in the system. This list is extended as
 * threads call locking functions and their locks are discovered. Note that new
 * locks are added to the front, which will become important later on.
 */
static struct list *ordering;
static pthread_mutex_t ordering_lock;

/* Whether we've triggered an exception. */
static int panic;

/* Pointers to the real locking functions. We're about to do a sneaky trick and
 * shadow the locking functions so that any calls to them in the binary we run
 * will actually get redirected to our code which wraps the underlying library
 * functions. We need to maintain pointers to the library functions so that we
 * don't just end up calling ourself when we try to do locking.
 */
static void *libpthread;
static int (*real_mutex_lock)(pthread_mutex_t*);
static int (*real_mutex_unlock)(pthread_mutex_t*);
static int (*real_mutex_trylock)(pthread_mutex_t*);
static int (*real_sem_wait)(sem_t*);
static int (*real_sem_post)(sem_t*);
static int (*real_sem_trywait)(sem_t*);

/* This function is called when the partial ordering is violated (or when any
 * critical setup or teardown functions fail). It is really static (don't try
 * and call this function directly), but not declared this way for ease of use
 * when working with a debugger. When you have a thorny locking problem, you'll
 * typically want to break on this function and introspect the state.
 */
void lock_monitor_exception(const char *format, ...)
    __attribute__((noreturn))
    __attribute__((format (printf, 1, 2)));
void lock_monitor_exception(const char *format, ...) {
    va_list ap;
    struct list *tl, *l;
    struct thread *t;

    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);

    /* Dump the information we have; analogous to a back trace in GDB. */
    fprintf(stderr, "Lock trace (threads reported as \"thread: locks "
            "held in reverse order...\"):\n");

    /* Thread list information. */
    /* Note that at the point the exception occurs a thread may be in the midst
     * of attempting to acquire a lock. If the call to lock has already occurred
     * but the underlying library locking call has not, the thread will be
     * reported as holding a lock that it actually does not yet hold. For this
     * reason it will sometimes look as if the constraints of a semaphore or
     * mutex have been exceeded although they have not.
     */
    real_mutex_lock(&thread_list_lock);
    tl = thread_list;
    real_mutex_unlock(&thread_list_lock);
    for (; tl; tl = tl->next) {
        t = (struct thread*)tl->value;
        fprintf(stderr, "%p: ", (void*)t->thread);
        for (l = t->locks; l; l = l->next)
            fprintf(stderr, "%p ", l->value);
        fprintf(stderr, "\n");
    }

    /* Ordering information. */
    real_mutex_lock(&ordering_lock);
    l = ordering;
    real_mutex_unlock(&ordering_lock);
    fprintf(stderr, "Lock ordering (reverse): ");
    for (; l; l = l->next)
        fprintf(stderr, "%p ", l->value);
    fprintf(stderr, "\n");

    panic = 1; /* Suppress warning messages during lock_monitor_destroy. */
    exit(1);
}

/* Setup resources for the instrumentation. Note, the GCC constructor attribute
 * ensures this function is called before anything else in this library. We
 * don't bother locking anything during this function because it is assumed
 * that we are running single-threaded at this point.
 */
void lock_monitor_init(void) __attribute__((constructor));
void lock_monitor_init(void) {
    /* Set up the function pointers. */
    libpthread = dlopen(LIBPTHREAD, RTLD_LAZY);
    if (!libpthread) {
        lock_monitor_exception("Lock Monitor: Failed to load libpthread: "
            "%s\n", dlerror());
        /* Unreachable. */
    }
    LOAD_FN(real_mutex_lock, libpthread, pthread_mutex_lock);
    LOAD_FN(real_mutex_unlock, libpthread, pthread_mutex_unlock);
    LOAD_FN(real_mutex_trylock, libpthread, pthread_mutex_trylock);
    LOAD_FN(real_sem_wait, libpthread, sem_wait);
    LOAD_FN(real_sem_post, libpthread, sem_post);
    LOAD_FN(real_sem_trywait, libpthread, sem_trywait);

    INIT_LOCK(ordering_lock);
    INIT_LOCK(thread_list_lock);
}

/* Release the instrumentation resources. Note, the GCC destructor attribute
 * ensures that this function is called just prior to unloading this library.
 * We don't bother locking anything because it is assumed we are running
 * single-threaded at this point.
 */
void lock_monitor_destroy(void) __attribute__((destructor));
void lock_monitor_destroy(void) {
    struct list *tl, *l, *temp;
    struct thread *t;

    /* Free thread_list. */
    tl = thread_list;
    while (tl) {
        t = (struct thread*)tl->value;
        /* Free the enclosed thread-specific locking information. */
        l = t->locks;
        if (l && !panic)
            fprintf(stderr, "Lock Monitor: Warning: Thread %p still holds "
               "locks when lock_monitor_destroy is running.\n",
               (void*)t->thread);
        while (l) {
            temp = l;
            l = l->next;
            free(temp);
        }
        free(t);
        temp = tl;
        tl = tl->next;
        free(temp);
    }
    thread_list = NULL;
    DESTROY_LOCK(thread_list_lock);

    /* Free ordering. */
    l = ordering;
    while (l) {
        temp = l;
        l = l->next;
        free(temp);
    }
    ordering = NULL;
    DESTROY_LOCK(ordering_lock);

    /* Clean up function pointers. */
    real_mutex_lock = NULL;
    real_mutex_unlock = NULL;
    real_sem_wait = NULL;
    real_sem_post = NULL;
    if (dlclose(libpthread))
        lock_monitor_exception("Failed to close libpthread: %s\n", dlerror());
        /* Unreachable. */
}

/* Adds an item to the ordering list (the partial ordering). Note that the
 * partial ordering is maintained back-to-front for efficiency (i.e. locks
 * later in the list are actually ones that should be grabbed earlier). With
 * this in mind, it is expected that the lock you are adding is after every
 * currently known lock in the partial ordering. It is assumed that you hold
 * ordering_lock when calling this function.
 */
static void add(const void *lock) {
    struct list *l;

    l = (struct list*)malloc(sizeof(struct list));
    l->value = lock;
    l->next = ordering;
    ordering = l;
}

/* Find the current thread in thread_list. If there is no entry for it yet,
 * this is created. Note the locking in this function that looks incorrect. We
 * cheat and only lock thread_list_lock when we are explicitly reading or
 * writing to thread_list. Because nodes are never removed from thread_list and
 * new nodes are only ever added to the front of the list we will never get an
 * inconsistent view of the list as long as no one reads/writes the actual
 * variable thread_list while we are also doing so (see, I told you that
 * add-to-front property would come up later ;).
 */
static struct thread *find_thread(void) {
    struct list *tl;
    pthread_t me;
    struct thread *t;

    me = pthread_self();
    real_mutex_lock(&thread_list_lock);
    tl = thread_list;
    real_mutex_unlock(&thread_list_lock);
    for (; tl; tl = tl->next) {
        t = (struct thread*)tl->value;
        if (pthread_equal(me, t->thread))
            /* Found ourselves. */
            return t;
    }

    /* We didn't find the thread struct, so let's create it. */
    t = (struct thread*)malloc(sizeof(struct thread));
    t->thread = me;
    t->locks = NULL;
    tl = (struct list*)malloc(sizeof(struct list));
    tl->value = (void*)t;
    real_mutex_lock(&thread_list_lock);
    tl->next = thread_list;
    thread_list = tl;
    real_mutex_unlock(&thread_list_lock);
    return t;
}

/* Check whether the lock given is already in the partial ordering (i.e.
 * whether we are already aware of it). It is assumed that the callee holds
 * ordering_lock when calling this function. It would have been possible to do
 * the ordering_lock locking and unlocking within this function, but this
 * doesn't actually work. We know that this function is called as a check
 * before calling add. Putting the locking inside here would leave a time
 * between calling known and add during which ordering_lock was not held. This
 * introduces a time-of-check-to-time-of-use problem. Consider the following:
 *   thread 1   thread 2
 *   --------   --------       Thread 1 and 2 can perform the known check for
 *    known?                   the same lock, get the same answer and then both
 *               known?        add this same lock. Hmm, lengthy comment for
 *                add          such a short function...
 *     add
 */
static int known(const void *lock) {
    struct list *l;

    for (l = ordering; l; l = l->next) {
        if (l->value == lock)
            return 1;
    }
    return 0;
}

/* Compare two locks via the partial ordering. Both locks must be in the
 * partial ordering already. The exception is when b is NULL, in which case it
 * will be absent. It is not assumed that the callee holds any locks. Returns
 * <0 when a is lesser, >0 when b is lesser, 0 when the locks are the same. As
 * with thread_list, we exploit the fact that we know nodes are not removed
 * from ordering and are only ever added to the front of the list. Therefore it
 * is OK to only lock ordering_lock on direct reading from ordering.
 */
static int lockcmp(const void *a, const void *b) {
    struct list *l;

    /* Optimisation to avoid locking. */
    if (a == b) return 0;

    real_mutex_lock(&ordering_lock);
    assert(known(a));
    assert(!b || known(b));
    l = ordering;
    real_mutex_unlock(&ordering_lock);

    for (; l; l = l->next) {
        if      (l->value == a) return 1;
        else if (l->value == b) return -1;
    }

    /* If we've reached this point than neither lock was found in the ordering
     * information, which means something has gone unexpectedly wrong (the
     * pre-loop assertions should have caught this).
     */
    lock_monitor_exception("Lock Monitor: lockcmp failure while comparing "
        "locks %p and %p.\n", a, b);
    /* Unreachable. */
    return 0;
}

/* Register with the internal data structures that we are about to lock a
 * semaphore/mutex. This is our chance to cacth a thread red-handed if it is
 * grabbing a lock out of order.
 */
static void lock(const void *lock) {
    struct thread *t;

    /* If we haven't seen the lock before, assume that it comes after every
     * other lock currently in the partial ordering. Note that this is the only
     * place locks are added to ordering. In other words, if you want to
     * explicitly define a partial ordering you may want to immediately lock
     * all your semaphores/mutexes on creation in the order you want to define,
     * unlock them, and only then commence using them in your parallel code.
     */
    real_mutex_lock(&ordering_lock);
    if (!known(lock))
        add(lock);
    assert(known(lock));
    real_mutex_unlock(&ordering_lock);

    t = find_thread();
    assert(t);

    /* Modify the check below to <= 0 if you want to prevent your semaphores
     * mutexes from being re-entrant (nested). Note that we don't do any
     * locking on ->locks even though we do all manner of operations on it.
     * This is not a mistake. We will only ever be reading/modifying ->locks
     * from the thread associated with this item in thread_list and hence there
     * will never be any concurrent access.
     */
    if (t->locks && lockcmp(lock, t->locks->value) < 0)
        lock_monitor_exception("Lock Monitor: Thread %p attempted to lock "
            "%p while holding the lock %p, that comes after it in the partial "
            "ordering.\n", (void*)t->thread, lock, t->locks->value);
        /* Unreachable. */
    else {
        /* Allow the lock to succeed. */
        struct list *l;

        l = (struct list*)malloc(sizeof(struct list));
        l->value = lock;
        l->next = t->locks;
        t->locks = l;
    }
}

/* The unlocking code is somewhat simpler because we don't need to do any
 * checks. If necessary you can modify this code to add checks to force the
 * order in which threads unlock variables.
 */
static void unlock(const void *lock) {
    struct thread *t;
    struct list *l, *temp;

    assert(known(lock)); /* No need to lock, as we're not modifying. */

    t = find_thread();
    assert(t);

    for (l = t->locks, temp = NULL; l; l = l->next) {
        if (l->value == lock) {
            if (!temp) t->locks = l->next;
            else       temp->next = l->next;
            free(l);
            return;
        }
    }

    /* We didn't find the lock. */
    lock_monitor_exception("Lock Monitor: Thread %p attempted to unlock %p "
        "while not holding this lock.\n", (void*)t->thread, lock);
    /* Unreachable. */
}

/* The wrapper functions that trap your binary's calls to the locking/unlocking
 * functions. The general pattern for locking operations is:
 *  1. Assess the lock against the partial ordering (call lock);
 *  2. Lock the variable;
 *  3. If the locking failed, remove it from the thread's lock list;
 *  4. Return the result.
 * The general pattern for unlocking is:
 *  1. Unlock the variable;
 *  2. If this SUCCEEDED, remove it from the thread's lock list;
 *  3. Return the result.
 */
int pthread_mutex_lock(pthread_mutex_t *mutex) {
    int ret;
    lock((void*)mutex);
    assert(real_mutex_lock);
    ret = real_mutex_lock(mutex);
    if (ret) unlock((void*)mutex);
    return ret;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex) {
    int ret;
    assert(real_mutex_unlock);
    ret = real_mutex_unlock(mutex);
    if (!ret) unlock((void*)mutex);
    return ret;
}

int pthread_mutex_trylock(pthread_mutex_t *mutex) {
    int ret;
    lock((void*)mutex);
    assert(real_mutex_trylock);
    ret = real_mutex_trylock(mutex);
    if (ret) unlock((void*)mutex);
    return ret;
}

int sem_wait(sem_t *sem) {
    int ret;
    lock((void*)sem);
    assert(real_sem_wait);
    ret = real_sem_wait(sem);
    if (ret) unlock((void*)sem);
    return ret;
}

int sem_post(sem_t *sem) {
    int ret;
    assert(real_sem_post);
    ret = real_sem_post(sem);
    if (!ret) unlock((void*)sem);
    return ret;
}

int sem_trywait(sem_t *sem) {
    int ret;
    lock((void*)sem);
    assert(real_sem_trywait);
    ret = real_sem_trywait(sem);
    if (ret) unlock((void*)sem);
    return ret;
}
