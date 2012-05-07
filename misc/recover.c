/* This is a quick hack I wrote to recover a file from a damaged file system.
 * It is designed to split into two processes, a monitor and a copier. The
 * copier does the actual recovery, but it is assumed that any of the copier's
 * operations on the file you are recovering could fail or block indefinitely.
 * The monitor checks that the copier is still operating correctly and, if not
 * kills and restarts it. This code has not been thoroughly tested and probably
 * contains lots of bugs.
 */

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

/* Print an error message and exit. */
#define DIE(args...) \
    do { \
        fprintf(stderr, args); \
        exit(1); \
    } while(0)

/* The time in seconds for the monitor to wait between checking for updates
 * from the copier. If the copier has not updated its status flag within
 * this time period, the monitor considers it to have malfunctioned and kills
 * it.
 */
#define WATCHDOG_TIME 1 /* seconds */

/* Number of bytes for the copier to read in between sending pulses back to the
 * monitor to indicate it is still operating correctly.
 */
#define HEART_RATE 100 /* bytes */

/* The maximum number of times to attempt to read an individual byte before
 * considering it to have failed.
 */
#define MAX_ATTEMPTS 10

/* The current status of the child process. */
static enum child_status {
    UNSTARTED,
    RUNNING,
    DEAD,
    COMPLETED,
} status = UNSTARTED;

/* Signal handler. Execution of the monitor arrives here when the copier sends
 * it a SIGUSR1 to indicate it is still operating correctly or SIGUSR2 to
 * indicate it has completed.
 */
static void signal_handler(int sig) {
    /* The main thread blocks SIGUSR* when updating status and this handler is
     * installer to block SIGUSR* when it is running, so we can assume that we
     * have exclusive access to status at this point.
     */
    switch (sig) {
        case SIGUSR1: {
            status = RUNNING;
            break;
        } case SIGUSR2: {
            status = COMPLETED;
            break;
        } default: {
            assert(!"Unexpected signal caught!");
        }
    }
}

/* Create a set of signals that contains SIGUSR1 and SIGUSR2. */
static inline int create_signal_set(sigset_t *set) {
    return (sigemptyset(set) ||
            sigaddset(set, SIGUSR1) ||
            sigaddset(set, SIGUSR2));
}

static int install_signal_handlers(sigset_t *const set) {
    struct sigaction handler = {
        .sa_handler = &signal_handler,
        .sa_mask = *set,
    };
    return (sigaction(SIGUSR1, &handler, NULL) ||
            sigaction(SIGUSR2, &handler, NULL));
}

static inline int block_signals(sigset_t *usr_signals) {
    return sigprocmask(SIG_BLOCK, usr_signals, NULL);
}

static inline int unblock_signals(sigset_t *usr_signals) {
    return sigprocmask(SIG_UNBLOCK, usr_signals, NULL);
}

/* Print program usage information. */
static void usage(char *progname) {
    fprintf(stderr,
        "%s options\n"
        "Utility for recovering data from corrupted files or faulty hardware.\n"
        " -b bytes | --bytes=bytes   Read only the specified number of bytes.\n"
        " -i file | --input=file     Read from the specified file/device.\n"
        " -l file | --log=file       Use the specified file for checkpointing.\n"
        " -O bytes | --offset=bytes  Start reading at bytes into the file.\n"
        " -o file | --output=file    Write to the specified file/device.\n",
        progname);
}

int main(int argc, char **argv) {
    struct option OPTIONS[] = {
        { "bytes", required_argument, 0, 'b' },
        { "input", required_argument, 0, 'i' },
        { "log", required_argument, 0, 'l' },
        { "offset", required_argument, 0, 'O' },
        { "output", required_argument, 0, 'o' },
        { 0, 0, 0, 0 },
    };
    int c, ignored, error;
    unsigned long long bytes = 0, offset = 0;
    char *input = NULL, *output = NULL, *log = NULL;
    sigset_t usr_signals;

    /* Parse command line arguments. */
    while ((c = getopt_long(argc, argv, "b:i:l:O:o:", OPTIONS, &ignored)) != -1) {
        switch (c) {
            case 'b': {
                bytes = atoll(optarg);
                break;
            } case 'i': {
                input = strdup(optarg);
                break;
            } case 'l': {
                log = strdup(optarg);
                break;
            } case 'O': {
                offset = atoll(optarg);
                break;
            } case 'o': {
                output = strdup(optarg);
                break;
            } default: {
                usage(argv[0]);
                return 1;
            }
        }
    }

    /* Check we got passed all the options we need. */
    if (input == NULL) {
        DIE("No input file specified.\n");
    } else if (output == NULL) {
        DIE("No output file specified.\n");
    } else if (log == NULL) {
        DIE("No log file specified.\n");
    }

    /* Setup the signal handlers here (before forking) because otherwise we
     * have a race where the child can signal the parent before it has the
     * handlers in place and cause it to exit. This is awkward because we then
     * have to remove them in the child immediately after forking.
     */
    if (create_signal_set(&usr_signals)) {
        DIE("Failed to create signal set.\n");
    }
    if (install_signal_handlers(&usr_signals)) {
        DIE("Failed to install signal handlers.\n");
    }

    while (1) {
        pid_t child = fork();

        if (child == -1) {
            DIE("Failed to fork (%d).\n", errno);

        } else if (child == 0) {
            /* We are the child (copier). */
            /* FIXME: separate this into another function. */
            FILE *in_fd = NULL, *out_fd = NULL, *log_fd = NULL;
            unsigned long long pos = 0, init_pos;
            pid_t monitor = getppid();
            struct stat ignored;
            unsigned char b;
            int attempts = 0;

            /* Remove the signal handlers. Failing to remove these has no
             * critical effect as we're not expecting to receive them, but it
             * seems cleaner to remove them.
             */
            if (signal(SIGUSR1, SIG_DFL) == SIG_ERR) {
                fprintf(stderr, "Warning: could not remove SIGUSR1 handler in child.\n");
            }
            if (signal(SIGUSR2, SIG_DFL) == SIG_ERR) {
                fprintf(stderr, "Warning: could not remove SIGUSR2 handler in child.\n");
            }

            if (stat(log, &ignored) == 0) {
                /* The log already exists. Let's try to resume. */
                if ((log_fd = fopen(log, "r")) == NULL) {
                    fprintf(stderr, "Log exists, but could not be opened to resume.\n");
                    goto complete;
                }
                /* Ignore failure to read. If we have no previous marker than
                 * the logical thing to do is start from the beginning.
                 */
                (void)fscanf(log_fd, "%llu", &pos);
                fclose(log_fd);
                log_fd = NULL; /* Reset to leave us in a consistent state for
                                * jumping to complete.
                                */
            }
            
            if (pos == 0 && offset != 0) {
                /* Position has not yet been set (by resuming) and the user
                 * wanted to start at a specific offset.
                 */
                pos = offset;
            }

            /* Now that we've see the position we're reading from, we need to
             * truncate the output if it's already exceeding this.
             */
            (void)truncate(output, pos);

            /* Keep track of the initial position (current size of the output)
             * for the purpose of fseeking out_fd.
             */
            init_pos = pos;

            if ((in_fd = fopen(input, "r")) == NULL) {
                fprintf(stderr, "Failed to open %s.\n", input);
                goto complete;
            }

            if ((out_fd = fopen(output, "a")) == NULL) {
                fprintf(stderr, "Failed to open %s.\n", output);
                goto complete;
            }

            if ((log_fd = fopen(log, "w")) == NULL) {
                fprintf(stderr, "Could not open log %s.\n", log);
                goto complete;
            }

            do {

                /* First note the byte we are about to copy. */
                if (fseek(log_fd, 0, SEEK_SET)) {
                    fprintf(stderr, "Failed to fseek the log %s.\n", log);
                    goto complete;
                }
                fprintf(log_fd, "%llu", pos);

                /* Now seek the input and output files (not necessary in all
                 * circumstances, but nice for consistency and also more
                 * reliable if the disk is corrupted).
                 */
                if (fseek(in_fd, pos, SEEK_SET)) {
                    fprintf(stderr, "Failed to fseek to %llu.\n", pos);
                    goto complete;
                }
                if (fseek(out_fd, pos - init_pos, SEEK_SET)) {
                    fprintf(stderr, "Failed to fseek the output %s.\n", output);
                    goto complete;
                }

                /* Check we haven't exceeded the maximum attempts. */
                ++attempts;
                if (attempts > MAX_ATTEMPTS) {
                    fprintf(stderr, "Warning: failing byte %llu in %s.\n", pos, input);
                } else {
                    /* Copy the byte we're up to. */
                    switch (fscanf(in_fd, "%c", &b)) {
                        case 1: {
                            /* Normal case; read a byte. */
                            fprintf(out_fd, "%c", b);
                            break;
                        } case EOF: {
                            fclose(log_fd);
                            log_fd = NULL;
                            if (unlink(log)) {
                                fprintf(stderr, "Warning: failed to delete log %s.\n", log);
                            }
                            goto complete;
                        } default: {
                            fprintf(stderr, "Warning: failed to read byte %llu from %s (re-trying).\n", pos, input);
                            continue;
                        }
                    }
                }
                pos++;
                /* Reset attempts as we successfully read a byte if we've
                 * reached here.
                 */
                attempts = 0;

                /* Send a heartbeat to the monitor if necessary. */
                if ((pos - offset) % HEART_RATE == 0) {
                    kill(monitor, SIGUSR1);
                }

                /* If the user specified a limit on the number of bytes to
                 * recover, check whether we have hit that limit.
                 */
                if (bytes != 0) {
                    if (pos - offset >= bytes) {
                        fclose(log_fd);
                        log_fd = NULL;
                        if (unlink(log)) {
                            fprintf(stderr, "Warning: failed to delete log %s.\n", log);
                        }
                        goto complete;
                    }
                }
            } while (1);
            assert(!"unreachable");

complete:
            /* Jump to here when we want to (deliberately) exit the copier. */

            /* Close any file descriptors as required. */
            if (log_fd) fclose(log_fd);
            if (out_fd) fclose(out_fd);
            if (in_fd) fclose(in_fd);

            /* Signal the monitor that we are done. */
            kill(monitor, SIGUSR2);

            /* We could return failure based on what happened, but the monitor
             * doesn't currently look for this anyway.
             */
            return 0;

        } else {
            /* We are the parent (the monitor). */
            unsigned int sleep_time;

            /* Wait to notice a lack of response (failure to reset the status
             * flag) or completion of the child.
             */
            error = block_signals(&usr_signals);
            assert(error == 0);
            do {
                status = DEAD;
                error = unblock_signals(&usr_signals);
                assert(error == 0);

                sleep_time = WATCHDOG_TIME;
                while (sleep_time > 0) {
                    sleep_time = sleep(sleep_time);
                }
                error = block_signals(&usr_signals);
                assert(error == 0);
            } while (status == RUNNING);
            
            if (status == DEAD) {
                /* The child stalled or died. Kill it if it's still running and
                 * ignore failure because we don't care if we're kicking its
                 * corpse.
                 */
                (void)kill(child, SIGKILL);
            } else {
                /* The child finished its task. */
                assert(status == COMPLETED);
                printf("Recovery complete!\n");
                /* No need to unblock_signals() as we're about to exit. */
                break;
            }
            error = unblock_signals(&usr_signals);
            assert(error == 0);
        }
    }

    return 0;
}
