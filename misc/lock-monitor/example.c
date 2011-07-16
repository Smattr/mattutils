/* Example code that demonstrates a deadlock and explains how to use
 * lock-monitor to expose it.
 *
 * The code below presents the prototypical deadlock example. To compile the
 * code run:
 *  gcc -lpthread -o example example.c
 * Run ./example a lot of times. You will probably find that most of the time
 * it runs fine, but every now and then it doesn't finish. You may be lucky and
 * it always runs to completion. If so, try compiling with:
 *  gcc -lpthread -DFORCE_DEADLOCK -o example example.c
 * Now you should find it never finishes.
 *
 * If you can't immediately see where the deadlock is in the code below, you
 * will probably find it useful to try and track it down yourself before
 * reading the rest of these instructions. Try and figure out why the code can
 * deadlock and what property of it needs to be altered to prevent this.
 *
 * Spoiler: The possibility of deadlock exists because Alice and Bob are
 * locking m and m+1 in different orders. The lock occurs when Alice tries to
 * grab m+1 and Bob tries to grab m. Each already hold the lock that the other
 * wants.
 *
 * Now note how we can use lock-monitor to identify this. See the instructions
 * in lock-monitor.c for building it and then run:
 *  LD_PRELOAD=./lock-monitor.so ./example
 * You will find that lock-monitor catches the faulty locking behaviour. Note
 * that you can compile this example code with -DSEMAPHORE to use semaphores
 * instead of mutexes to much the same effect.
 *
 * As an aside, unfortunately the information the lock-monitor gives is rather
 * cryptic; memory and thread struct addresses rather than variable names. This
 * is because it has no real information about the internals of your program.
 * If you find lock-monitor identifying problems and you cannot spot the cause
 * you may find that you need to resort to a debugger like GDB to introspect
 * your code at the point as which lock_monitor_exception is called.
 */
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#ifdef SEMAPHORE
    /* Setup the locks as semaphores. */
    typedef sem_t      lock_t;
    #define INIT(l)    sem_init((l), 0, 1)
    #define DESTROY(l) sem_destroy(l)
    #define LOCK(l)    sem_wait(l)
    #define UNLOCK(l)  sem_post(l)
#else /* !SEMAPHORE */
    /* Setup the locks as mutexes. */
    typedef pthread_mutex_t lock_t;
    #define INIT(l)         pthread_mutex_init((l), NULL)
    #define DESTROY(l)      pthread_mutex_destroy(l)
    #define LOCK(l)         pthread_mutex_lock(l)
    #define UNLOCK(l)       pthread_mutex_unlock(l)
#endif

void alice_main(lock_t m[]) {
    LOCK(m);
#ifdef FORCE_DEADLOCK
    sleep(1);
#endif
    LOCK(m + 1);
    UNLOCK(m + 1);
    UNLOCK(m);
}

void bob_main(lock_t m[]) {
    LOCK(m + 1);
#ifdef FORCE_DEADLOCK
    sleep(1);
#endif
    LOCK(m);
    UNLOCK(m);
    UNLOCK(m + 1);
}

int main(int argc, char** argv) {
    lock_t m[2];
    pthread_t alice, bob;

    if (INIT(m)) {
        fprintf(stderr, "Failed to create first mutex.\n");
        return -1;
    }

    if (INIT(m + 1)) {
        fprintf(stderr, "Failed to create second mutex.\n");
        return -1;
    }

    if (pthread_create(&alice, NULL, (void *(*)(void*))alice_main, m)) {
        fprintf(stderr, "Failed to start alice.\n");
        return -1;
    }

    if (pthread_create(&bob, NULL, (void *(*)(void*))bob_main, m)) {
        fprintf(stderr, "Failed to start bob.\n");
        return -1;
    }

    pthread_join(alice, NULL);
    pthread_join(bob, NULL);
    DESTROY(m);
    DESTROY(m + 1);

    return 0;
}
