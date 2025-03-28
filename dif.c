/// Dif – Autopilot for diff|less
///
/// What does that mean?
///
///      start
///        │
///        ▼
///   ┌──────────┐   yes   ┌────────────────────┐
///   │argc > 1 ?├────────►│ replace stdin with │
///   └────┬─────┘         │diff -purNd argv[1:]│
///        │               └─────────┬──────────┘
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
#include <signal.h>
#include <spawn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef __APPLE__
#include <crt_externs.h>
#endif

/// process IDs for debugging emergencies
static pid_t diff_pid;
static pid_t less_pid;

/// kill children and undo damage they may have done
static void sanity(void) {
  // unceremoniously murder our children
  if (diff_pid != 0)
    kill(diff_pid, SIGKILL);
  if (less_pid != 0)
    kill(less_pid, SIGKILL);

  // undo any havoc they may have wrought, using stdout because stdin has
  // likely been remapped
  int r __attribute__((unused)) = system("stty --file=/dev/stdout sane");
}

/// assertion that anticipates TTY chaos
///
/// Under other programs, when failing an assertion a failure message is printed
/// and `abort` is called. For us, diff+less are using the terminal and may have
/// put it in a state where the failure message is invisible. This macro
/// attempts to recover from this ambiguous state and get us a readable failure.
#define ASSERT(expr)                                                           \
  do {                                                                         \
    if (!(expr)) {                                                             \
      sanity();                                                                \
      assert(expr);                                                            \
    }                                                                          \
  } while (0)

/// a “last gasp” debugging helper
#define SOS(...)                                                               \
  do {                                                                         \
    sanity();                                                                  \
    fprintf(stderr, "%s:%d: ", __FILE__, __LINE__);                            \
    fprintf(stderr, __VA_ARGS__);                                              \
    fflush(stderr);                                                            \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

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
  ASSERT(argc > 0);
  ASSERT(argv != NULL);

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
  diff_pid = pid;
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
  ASSERT(less != NULL);

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
  less_pid = pid;
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
  ASSERT(s != NULL);

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
  ASSERT(lines != NULL);
  ASSERT(line != NULL);

  if (lines->n_line == lines->c_line) {
    const size_t new_c = lines->c_line == 0 ? 128 : (lines->c_line * 2);
    char **const l = realloc(lines->line, new_c * sizeof(lines->line[0]));
    if (l == NULL)
      return ENOMEM;
    lines->c_line = new_c;
    lines->line = l;
  }

  ASSERT(lines->n_line < lines->c_line);
  lines->line[lines->n_line] = line;
  ++lines->n_line;

  return 0;
}

/// remove all entries from a list
static void lines_clear(lines_t *lines) {
  ASSERT(lines != NULL);

  for (size_t i = 0; i < lines->n_line; ++i)
    free(lines->line[i]);
  lines->n_line = 0;
}

/// remove all entries from a list and deallocate backing storage
static void lines_free(lines_t *lines) {
  ASSERT(lines != NULL);

  lines_clear(lines);
  free(lines->line);
  *lines = (lines_t){0};
}

/// the type of change a diff hunk represents
typedef enum { ADDED, MODIFIED, DELETED } transition_t;

/// diff lines that have been accrued but not yet output
typedef struct {
  char *heading;     ///< last “diff --git …” line we saw
  transition_t mode; ///< what kind of change is this hunk?
  lines_t neg;       ///< “deleted” lines
  lines_t pos;       ///< “added” lines
} pending_t;

/// is this a white space character we expect to encounter in a code line?
static bool is_space(char c) { return c == ' ' || c == '\t'; }

/// write out a diff line
///
/// If a `pair` is provided, this will attempt to do diff-so-fancy-style
/// highlighting within the diff line.
///
/// \param colourise Add ANSI colour to the output?
/// \param line Line to write
/// \param pair Optional matching inverse diff line
/// \param sink Output to write to
/// \return 0 on success or an errno on failure
static int flush_line(bool colourise, const char *line, const char *pair,
                      FILE *sink) {
  ASSERT(line != NULL);
  ASSERT(sink != NULL);
  ASSERT(pair == NULL || (line[0] == '+' && pair[0] == '-') ||
         (line[0] == '-' && pair[0] == '+'));

  // is the common prefix and suffix only white space?
  bool spaces = true;

  // find the common prefix
  size_t prefix = 1; // account for '+'/'-'
  if (pair != NULL) {
    for (size_t i = 1; line[i] != '\0' && line[i] == pair[i]; ++i) {
      spaces &= is_space(line[i]);
      ++prefix;
    }
  }

  // find the common suffix
  const size_t line_len = strlen(line);
  ASSERT(line_len > 0);
  size_t suffix = 0;
  if (pair != NULL) {
    const size_t pair_len = strlen(pair);
    ASSERT(pair_len > 0);
    for (size_t i = 0; i < line_len && i < pair_len &&
                       line[line_len - i - 1] == pair[pair_len - i - 1];
         ++i) {
      spaces &= is_space(line[line_len - i - 1]);
      ++suffix;
    }

    // If the suffix extends into the prefix, reduce it. This can happen in
    // diffs like:
    //   -foo(a, b)
    //   +foo(a, c, b)
    // Without this tweak, prefix would be 8 and suffix would be 5.
    if (prefix + suffix > line_len)
      suffix = line_len - prefix;
    if (prefix + suffix > pair_len)
      suffix = pair_len - prefix;
  }

  // should we perform word highlighting?
  const bool highlight_words =
      !spaces && // the common prefix/suffix is not just white space
      (prefix > 1 || suffix > 0) // we have a non-trivial common prefix/suffix
      && prefix != line_len;     // the lines do not somehow match exactly

  for (size_t i = 0; i < line_len; ++i) {

    if (colourise) {
      const char *esc = NULL;
      if (i == 0 && line[i] == '+') {
        esc = "\033[32m"; // green
      } else if (i == 0 && line[i] == '-') {
        esc = "\033[31m"; // red
      } else if (i == 0 && line[i] == '@') {
        esc = "\033[36m"; // cyan
      } else if (line[i] == '\n') {
        esc = "\033[0m"; // reset
      } else if (i == 0 && line[i] != ' ') {
        esc = "\033[1m"; // bold
      } else if (i != 0 && highlight_words) {
        if (i == prefix) {
          esc = "\033[7m"; // invert colours
        }
        // avoid `else if` in order to handle the scenario where `prefix +
        // suffix == line_len`
        if (line_len - i == suffix) {
          esc = "\033[27m"; // uninvert colours
        }
      }
      if (esc != NULL) {
        if (fputs(esc, sink) < 0)
          return EIO;
      }
    }

    if (fputc(line[i], sink) < 0)
      return EIO;

    // if this line was not properly terminated, ensure we return to a sane
    // state
    if (i + 1 == line_len && line[i] != '\n') {
      if (fputs("\033[0m", sink) < 0)
        return EIO;
    }
  }

  return 0;
}

/// write out diff lines
///
/// \param colourise Add ANSI colour to the output?
/// \param pending Negative and positive lines to write
/// \param sink Output to write to
/// \return 0 on success or an errno on failure
static int flush_lines(bool colourise, pending_t *pending, FILE *sink) {
  ASSERT(pending != NULL);
  ASSERT(sink != NULL);

  for (size_t i = 0; i < pending->neg.n_line; ++i) {
    const char *pair = NULL;
    if (pending->neg.n_line == pending->pos.n_line)
      pair = pending->pos.line[i];
    const int r = flush_line(colourise, pending->neg.line[i], pair, sink);
    if (r != 0)
      return r;
  }

  for (size_t i = 0; i < pending->pos.n_line; ++i) {
    const char *pair = NULL;
    if (pending->neg.n_line == pending->pos.n_line)
      pair = pending->neg.line[i];
    const int r = flush_line(colourise, pending->pos.line[i], pair, sink);
    if (r != 0)
      return r;
  }

  lines_clear(&pending->neg);
  lines_clear(&pending->pos);

  return 0;
}

static bool startswith(const char *s, const char *prefix) {
  ASSERT(s != NULL);
  ASSERT(prefix != NULL);
  return strncmp(s, prefix, strlen(prefix)) == 0;
}

/// write a diff-so-fancy style section header
///
/// \param mode Type of change being made to the upcoming file
/// \param heading The “diff --git …” line responsible for this header
/// \param sink Output to write to
/// \return 0 on success or an errno on failure
static int write_header(transition_t mode, const char *heading, FILE *sink) {
  ASSERT(heading != NULL);
  ASSERT(startswith(heading, "diff --git "));
  ASSERT(sink != NULL);

  // learn the width of the terminal
  static size_t width;
  if (width == 0) {
    struct winsize ws = {0};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) < 0)
      return errno;
    width = ws.ws_col;
  }

  // determine the path(s) to the file being changed
  const char *const file_pair = &heading[strlen("diff --git ")];
  const size_t file_pair_len = strlen(file_pair);
  ASSERT(file_pair[file_pair_len - 1] == '\n' &&
         "heading not \"diff --git …\\n\"");
  const char *const space = strchr(file_pair, ' ');
  ASSERT(space != NULL && "no ' ' in file pair section of \"diff --git …\"");

  const char *const from = file_pair;
  const size_t from_len = (size_t)(space - file_pair);
  const char *const to = space + 1;
  const size_t to_len = file_pair_len - from_len - 2;

  // if the file was moved, we may have two different paths
  const bool was_moved = from_len != to_len || strncmp(from, to, from_len) != 0;

  size_t j;
  if (mode == ADDED) {
    ASSERT(!was_moved);
    if (fputs("\033[32;7madded: \033[1m", sink) < 0)
      return EIO;
    j = strlen("added: ");
  } else if (mode == MODIFIED) {
    if (fputs("\033[33;7mmodified: \033[1m", sink) < 0)
      return EIO;
    j = strlen("modified: ");
  } else {
    ASSERT(!was_moved);
    if (fputs("\033[31;7mdeleted: \033[1m", sink) < 0)
      return EIO;
    j = strlen("deleted: ");
  }

  if (fprintf(sink, "%.*s", (int)from_len, from) < 0)
    return EIO;
  j += from_len;

  if (was_moved) {
    if (fprintf(sink, " → %.*s", (int)to_len, to) < 0)
      return EIO;
    j += 3 + to_len;
  }

  for (; j < width; ++j) {
    if (fputs(" ", sink) < 0)
      return EIO;
  }
  if (fputs("\033[0m\n", sink) < 0)
    return EIO;

  return 0;
}

int main(int argc, char **argv) {

  proc_t diff = {.in = -1, .out = STDIN_FILENO};
  proc_t less = {.in = STDOUT_FILENO, .out = -1};
  FILE *in = NULL;
  FILE *out = stdout;
  char *buffer = NULL;
  size_t buffer_size = 0;
  pending_t pending = {0};
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
        prelude &= !startswith(buffer, STARTERS[i]);
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
      if (buffer[0] == '-' && !startswith(buffer, "--- ") &&
          pending.pos.n_line == 0) {
        // a negative line that may have a later matching positive line
        const int r = lines_append(&pending.neg, buffer);
        if (r != 0) {
          fprintf(stderr, "failed to accumulate negative line: %s\n",
                  strerror(r));
          rc = EXIT_FAILURE;
          goto done;
        }
        buffer = NULL;
        buffer_size = 0;
        continue;
      } else if (buffer[0] == '+' && !startswith(buffer, "+++ ")) {
        // a positive line that may have an earlier matching negative line
        const int r = lines_append(&pending.pos, buffer);
        if (r != 0) {
          fprintf(stderr, "failed to accumulate positive line: %s\n",
                  strerror(r));
          rc = EXIT_FAILURE;
          goto done;
        }
        buffer = NULL;
        buffer_size = 0;
        continue;
      } else if (startswith(buffer, "rename ")) {
        // ignore notes about renames that we can infer from the diff header
        continue;
      }
    }

    // output any preceding lines to the one we are about to write
    {
      const int r = flush_lines(add_colour, &pending, out);
      if (r != 0) {
        fprintf(stderr, "failed to flush lines: %s\n", strerror(r));
        rc = EXIT_FAILURE;
        goto done;
      }
    }

    // if this is a diff leader, save it for later
    if (add_colour && startswith(buffer, "diff --git ")) {
      free(pending.heading);
      pending.heading = buffer;
      pending.mode = MODIFIED;
      buffer = NULL;
      buffer_size = 0;
      continue;
    }

    // if we see a mode line, update what kind of change this is
    if (add_colour && startswith(buffer, "new file ")) {
      pending.mode = ADDED;
      continue;
    }
    if (add_colour && startswith(buffer, "deleted file ")) {
      pending.mode = DELETED;
      continue;
    }

    // if we see an index line or a pure move, flush the heading
    if (add_colour &&
        (startswith(buffer, "index ") ||
         strcmp(buffer, "similarity index 100%\n") == 0) &&
        pending.heading != NULL) {
      const int r = write_header(pending.mode, pending.heading, out);
      if (r != 0) {
        fprintf(stderr, "failed to write header: %s\n", strerror(r));
        rc = EXIT_FAILURE;
        goto done;
      }
      continue;
    }

    if (add_colour) {
      // suppress the diff leader lines following the command
      if (startswith(buffer, "+++ "))
        continue;
      if (startswith(buffer, "--- "))
        continue;

      // suppress similarity lines
      if (startswith(buffer, "similarity "))
        continue;
    }

    // output our current line
    const int r = flush_line(!prelude && add_colour, buffer, NULL, out);
    if (r != 0) {
      fprintf(stderr, "failed to write line: %s\n", strerror(r));
      rc = EXIT_FAILURE;
      goto done;
    }
  }

  // output any trailing lines we buffered
  {
    const int r = flush_lines(add_colour, &pending, out);
    if (r != 0) {
      fprintf(stderr, "failed to flush lines: %s\n", strerror(r));
      rc = EXIT_FAILURE;
      goto done;
    }
  }

done:
  lines_free(&pending.pos);
  lines_free(&pending.neg);
  free(pending.heading);
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
