/// Dif – Autopilot for diff|less
///
/// What does that mean?
///
///      start
///        │
///        ▼
///   ┌──────────┐   yes   ┌───────────────────┐
///   │argc > 1 ?├────────►│replace stdin with │
///   └────┬─────┘         │diff -purN argv[1:]│
///        │               └─────────┬─────────┘
///        │ no                      │
///        │       ┌─────────────────┘
///        ▼       ▼
///   ┌───────────────┐   yes   ┌─────────────────────┐
///   │isatty(stdout)?│────────►│add ANSI colour codes│
///   └───────────────┘         └─────────────────────┘
///           │                            │
///           │ no                         │
///           │                            │
///           ▼                            ▼
///   ╔═════════════════╗            ╔════════════╗
///   ║strip ANSI colour║            ║pipe to less║
///   ║ codes and print ║            ╚════════════╝
///   ╚═════════════════╝
///
/// This code is in the public domain. Use it in any way you see fit.

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <spawn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef __APPLE__
#include <crt_externs.h>
#endif

/// get a pointer to `environ`
static char **get_environ(void) {
#ifdef __APPLE__
  // on macOS, environ is not directly accessible
  return *_NSGetEnviron();
#else
  // some platforms fail to expose environ in a header (e.g. FreeBSD), so
  // declare it ourselves and assume it will be available when linking
  extern char **environ;

  return environ;
#endif
}

/// a subprocess
typedef struct {
  pid_t pid; ///< identifier of the process (0 == not running)
  int in;    ///< stdin descriptor (<0 == unavailable)
  int out;   ///< stdout descriptor (<0 == unavailable)
} proc_t;

/// `pipe` that also sets close-on-exec
///
/// We avoid `pipe2` that could do this in one-shot due to portability issues.
static int pipe_(int fd[static 2]) {

  int out[] = {-1, -1};
  int rc = 0;

  if (pipe(out) < 0)
    return errno;

  for (size_t i = 0; i < sizeof(out) / sizeof(out[0]); ++i) {
    const int flags = fcntl(out[i], F_GETFD);
    if (fcntl(out[i], F_SETFD, flags | FD_CLOEXEC) < 0) {
      rc = errno;
      goto done;
    }
  }

  fd[0] = out[0];
  out[0] = -1;
  fd[1] = out[1];
  out[1] = -1;

done:
  for (size_t i = 0; i < sizeof(out) / sizeof(out[0]); ++i) {
    if (out[i] >= 0)
      (void)close(out[i]);
  }

  return rc;
}

/// start `diff -purNd argv…`
///
/// \param diff [out] Handle to the started process on success
/// \param argc Number of extra command line options to add
/// \param argv Extra command line options
/// \return 0 on success or an errno on failure
static int run_diff(proc_t *diff, int argc, char **argv) {
  assert(argc > 0);
  assert(argv != NULL);

  *diff = (proc_t){.in = -1, .out = -1};
  const char **args = NULL;
  posix_spawn_file_actions_t fa;
  int out[] = {-1, -1};
  int rc = 0;

  if ((rc = posix_spawn_file_actions_init(&fa)))
    return rc;

  if ((rc = pipe_(out)))
    goto done;

  if ((rc = posix_spawn_file_actions_adddup2(&fa, out[1], STDOUT_FILENO)))
    goto done;

  static const char *const PREFIX[] = {"diff",       "--show-c-function",
                                       "--unified",  "--recursive",
                                       "--new-file", "--minimal"};
  static const size_t PREFIX_SIZE = sizeof(PREFIX) / sizeof(PREFIX[0]);

  const size_t args_size = PREFIX_SIZE + (size_t)argc;
  args = calloc(args_size + 1, sizeof(args[0]));
  if (args == NULL) {
    rc = ENOMEM;
    goto done;
  }
  for (size_t i = 0; i < args_size; ++i) {
    if (i < PREFIX_SIZE) {
      args[i] = PREFIX[i];
    } else {
      args[i] = argv[i - PREFIX_SIZE];
    }
  }

  pid_t pid;
  if ((rc = posix_spawnp(&pid, args[0], &fa, NULL, (char *const *)args,
                         get_environ())))
    goto done;

  // success
  *diff = (proc_t){.pid = pid, .in = -1, .out = out[0]};
  out[0] = -1;

done:
  free(args);
  if (out[0] >= 0)
    (void)close(out[0]);
  if (out[1] >= 0)
    (void)close(out[1]);
  (void)posix_spawn_file_actions_destroy(&fa);

  return rc;
}

/// start `less -FRX`
///
/// \param less [out] Handle to the started process on success
/// \return 0 on success or an errno on failure
static int run_less(proc_t *less) {
  assert(less != NULL);

  *less = (proc_t){.in = -1, .out = -1};
  posix_spawn_file_actions_t fa;
  int out[] = {-1, -1};
  int rc = 0;

  if ((rc = posix_spawn_file_actions_init(&fa)))
    return rc;

  if ((rc = pipe_(out)))
    goto done;

  if ((rc = posix_spawn_file_actions_adddup2(&fa, out[0], STDIN_FILENO)))
    goto done;

  const char *const args[] = {
      "less", "--RAW-CONTROL-CHARS", "--quit-if-one-screen", "--no-init", "+Gg",
      NULL};

  pid_t pid;
  if ((rc = posix_spawnp(&pid, args[0], &fa, NULL, (char *const *)args,
                         get_environ())))
    goto done;

  // success
  *less = (proc_t){.pid = pid, .in = out[1], .out = -1};
  out[1] = -1;

done:
  if (out[0] >= 0)
    (void)close(out[0]);
  if (out[1] >= 0)
    (void)close(out[1]);
  (void)posix_spawn_file_actions_destroy(&fa);

  return rc;
}

/// strip ANSI colour codes from a string
static void decolourise(char *s) {
  assert(s != NULL);

  bool dropping = false;
  for (size_t dst = 0, src = 0;; ++src) {
    if (!dropping && s[src] == '\033')
      dropping = true;
    if (!dropping)
      s[dst++] = s[src];
    if (dropping && s[src] == 'm')
      dropping = false;
    if (s[src] == '\0')
      break;
  }
}

/// a collection of accumulated lines
typedef struct {
  char **line;   ///< line data
  size_t n_line; ///< number of entries in `line`
  size_t c_line; ///< number of available backing entries in `line`
} lines_t;

/// add a new line to a list
///
/// \param lines List to add to
/// \param line Line to add
/// \return 0 on success or an errno on failure
static int lines_append(lines_t *lines, char *line) {
  assert(lines != NULL);
  assert(line != NULL);

  if (lines->n_line == lines->c_line) {
    size_t new_c = lines->c_line == 0 ? 128 : (lines->c_line * 2);
    char **l = realloc(lines->line, new_c * sizeof(lines->line[0]));
    if (l == NULL)
      return ENOMEM;
    lines->c_line = new_c;
    lines->line = l;
  }

  assert(lines->n_line < lines->c_line);
  lines->line[lines->n_line] = line;
  ++lines->n_line;

  return 0;
}

/// remove all entries from a list
static void lines_clear(lines_t *lines) {
  assert(lines != NULL);

  for (size_t i = 0; i < lines->n_line; ++i)
    free(lines->line[i]);
  lines->n_line = 0;
}

/// remove all entries from a list and deallocate backing storage
static void lines_free(lines_t *lines) {
  assert(lines != NULL);

  lines_clear(lines);
  free(lines->line);
  *lines = (lines_t){0};
}

/// parsing state
typedef struct {
  lines_t pending_neg; ///< queued “deleted” lines to be output
  lines_t pending_pos; ///< queued “added” lines to be output
} state_t;

/// write out a diff line
///
/// \param colourise Add ANSI colour to the output?
/// \param line Line to write
/// \param sink Output to write to
/// \return 0 on success or an errno on failure
static int flush_line(bool colourise, const char *line, FILE *sink) {
  assert(line != NULL);
  assert(sink != NULL);

  for (size_t i = 0; line[i] != '\0'; ++i) {

    if (colourise) {
      const char *esc = NULL;
      if (i == 0 && line[i] == '+' && line[i + 1] != '+') {
        esc = "\033[32m"; // green
      } else if (i == 0 && line[i] == '-' && line[i + 1] != '-') {
        esc = "\033[31m"; // red
      } else if (i == 0 && line[i] == '@') {
        esc = "\033[36m"; // cyan
      } else if (line[i] == '\n') {
        esc = "\033[0m"; // reset
      } else if (i == 0 && line[i] != ' ') {
        esc = "\033[1m"; // bold
      }
      if (esc != NULL) {
        if (fputs(esc, sink) < 0)
          return EIO;
      }
    }

    if (fputc(line[i], sink) < 0)
      return EIO;
  }

  return 0;
}

/// write out diff lines
///
/// \param colourise Add ANSI colour to the output?
/// \param state Negative and positive lines to write
/// \param sink Output to write to
/// \return 0 on success or an errno on failure
static int flush_lines(bool colourise, state_t *state, FILE *sink) {
  assert(state != NULL);
  assert(sink != NULL);

  for (size_t i = 0; i < state->pending_neg.n_line; ++i) {
    const int r = flush_line(colourise, state->pending_neg.line[i], sink);
    if (r != 0)
      return r;
  }
  lines_clear(&state->pending_neg);

  for (size_t i = 0; i < state->pending_pos.n_line; ++i) {
    const int r = flush_line(colourise, state->pending_pos.line[i], sink);
    if (r != 0)
      return r;
  }
  lines_clear(&state->pending_pos);

  return 0;
}

int main(int argc, char **argv) {

  proc_t diff = {.in = -1, .out = STDIN_FILENO};
  proc_t less = {.in = STDOUT_FILENO, .out = -1};
  FILE *in = NULL;
  FILE *out = stdout;
  char *buffer = NULL;
  size_t buffer_size = 0;
  state_t state = {0};
  int rc = EXIT_SUCCESS;

  if (argc > 1) {
    const int r = run_diff(&diff, argc - 1, argv + 1);
    if (r != 0) {
      fprintf(stderr, "failed to run diff: %s\n", strerror(r));
      rc = EXIT_FAILURE;
      goto done;
    }
  }

  // decide whether to colourise the output
  const bool add_colour = isatty(STDOUT_FILENO);

  // wrap input in `FILE *` to permit `getline`
  in = fdopen(diff.out, "r");
  if (in == NULL) {
    fprintf(stderr, "failed to fdopen: %s\n", strerror(errno));
    rc = EXIT_FAILURE;
    goto done;
  }
  diff.out = -1;

  for (bool prelude = true;;) {

    errno = 0;
    if (getline(&buffer, &buffer_size, in) < 0) {
      if (errno == EINTR)
        continue;
      if (errno != 0) {
        fprintf(stderr, "failed to read from diff: %s\n", strerror(errno));
        rc = EXIT_FAILURE;
        goto done;
      }
      break;
    }

    decolourise(buffer);

    // are we exiting the prelude and into the diff context?
    if (prelude) {
      static const char *const STARTERS[] = {"diff ", "index ", "+++ ", "--- "};
      for (size_t i = 0; i < sizeof(STARTERS) / sizeof(STARTERS[0]); ++i)
        prelude &= strncmp(buffer, STARTERS[i], strlen(STARTERS[i])) != 0;
    }

    if (add_colour) {

      // Do we need to start less? We start it here so as to avoid running
      // it (and clearing the screen) if the diff is empty.
      if (less.pid == 0) {
        const int r = run_less(&less);
        if (r != 0) {
          fprintf(stderr, "failed to run less: %s\n", strerror(errno));
          rc = EXIT_FAILURE;
          goto done;
        }

        // wrap output in `FILE *` to avoid dealing with EINTR etc
        out = fdopen(less.in, "w");
        if (out == NULL) {
          fprintf(stderr, "failed to fdopen: %s\n", strerror(errno));
          rc = EXIT_FAILURE;
          goto done;
        }
        less.in = -1;
      }
    }

    // accumulate this line if we can later output it with more contextual hints
    if (!prelude) {
      if (buffer[0] == '-' && buffer[1] != '-' &&
          state.pending_pos.n_line == 0) {
        // a negative line that may have a later matching positive line
        const int r = lines_append(&state.pending_neg, buffer);
        if (r != 0) {
          fprintf(stderr, "failed to accumulate negative line: %s\n",
                  strerror(r));
          rc = EXIT_FAILURE;
          goto done;
        }
        buffer = NULL;
        buffer_size = 0;
        continue;
      } else if (buffer[0] == '+' && buffer[1] != '+') {
        // a positive line that may have an earlier matching negative line
        const int r = lines_append(&state.pending_pos, buffer);
        if (r != 0) {
          fprintf(stderr, "failed to accumulate positive line: %s\n",
                  strerror(r));
          rc = EXIT_FAILURE;
          goto done;
        }
        buffer = NULL;
        buffer_size = 0;
        continue;
      }
    }

    // output any preceding lines to the one we are about to write
    {
      const int r = flush_lines(add_colour, &state, out);
      if (r != 0) {
        fprintf(stderr, "failed to flush lines: %s\n", strerror(r));
        rc = EXIT_FAILURE;
        goto done;
      }
    }

    // output our current line
    const int r = flush_line(!prelude && add_colour, buffer, out);
    if (r != 0) {
      fprintf(stderr, "failed to write line: %s\n", strerror(r));
      rc = EXIT_FAILURE;
      goto done;
    }
  }

  // output any trailing lines we buffered
  {
    const int r = flush_lines(add_colour, &state, out);
    if (r != 0) {
      fprintf(stderr, "failed to flush lines: %s\n", strerror(r));
      rc = EXIT_FAILURE;
      goto done;
    }
  }

done:
  lines_free(&state.pending_pos);
  lines_free(&state.pending_neg);
  free(buffer);

  // close our pipe to prompt `diff` to exit
  if (in != NULL)
    (void)fclose(in);
  if (diff.out >= 0)
    (void)close(diff.out);

  // clean up `diff`
  do {
    if (diff.pid > 0) {

      // wait for it to exit
      int status;
      if (waitpid(diff.pid, &status, 0) < 0) {
        fprintf(stderr, "failed to clean up diff: %s\n", strerror(errno));
        rc = EXIT_FAILURE;
        break;
      }

      // propagate its exit status, only if `less`’s will not subsume it
      if (WIFEXITED(status) && rc == EXIT_SUCCESS && less.pid <= 0)
        rc = WEXITSTATUS(status);
    }
  } while (0);

  // close our pipe to prompt `less` to exit
  if (out != NULL)
    (void)fclose(out);
  if (less.in >= 0)
    (void)close(less.in);

  // clean up `less`
  do {
    if (less.pid > 0) {

      // wait for it to exit
      int status;
      if (waitpid(less.pid, &status, 0) < 0) {
        fprintf(stderr, "failed to clean up less: %s\n", strerror(errno));
        if (rc == EXIT_SUCCESS)
          rc = EXIT_FAILURE;
        break;
      }

      // propagate its exit status
      if (WIFEXITED(status) && rc == EXIT_SUCCESS)
        rc = WEXITSTATUS(status);
    }
  } while (0);

  return rc;
}
