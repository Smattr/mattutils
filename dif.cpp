/* Dif - Autopilot for diff|less
 *
 * What does that mean?
 *
 *     start
 *       ↓
 *  ┌──────────┐   yes   ┌───────────────────┐
 *  │argc > 1 ?├────────→│replace stdin with │
 *  └────┬─────┘         │diff -purN argv[1:]│
 *       │               └─────────┬─────────┘
 *       │ no                      │
 *       │       ┌─────────────────┘
 *       ↓       ↓
 *  ┌───────────────┐   yes   ┌─────────────────────┐
 *  │isatty(stdout)?│────────→│add ANSI colour codes│
 *  └───────────────┘         └─────────────────────┘
 *          │                            │
 *          │ no                         │
 *          │                            │
 *          ↓                            ↓
 *  ╔═════════════════╗       ┌────────────────────┐   yes   ┌─────────────┐
 *  ║strip ANSI colour║       │which diff-so-fancy?│────────→│pipe through │
 *  ║ codes and print ║       └────────────────────┘         │diff-so-fancy│
 *  ╚═════════════════╝                  │                   └─────────────┘
 *                                       │ no                       │
 *                                       │    ┌─────────────────────┘
 *                                       ↓    ↓
 *                                 ╔════════════╗
 *                                 ║pipe to less║
 *                                 ╚════════════╝
 *
 * This code is in the public domain. Use it in any way you see fit.
 */

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <signal.h>
#include <fcntl.h>
#include <new>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

static bool which(const char *command) {
  char *cmd;
  if (asprintf(&cmd, "which \"%s\" >/dev/null 2>/dev/null", command) < 0)
    return false;
  int r = system(cmd);
  free(cmd);
  return r == 0;
}

// A child process
class Child {

 public:
  Child(const char **argv, int cin = -1, int cout = -1);
  int kill() const;
  int wait();
  bool good() const { return m_pid >= 0; }
  int cin() const { return m_cin; }
  int cout() const { return m_cout; }
  int cin_cloexec();
  int cout_cloexec();
  void hup();
  ~Child();

 private:
  int m_cin;
  int m_cout;
  pid_t m_pid;

};

Child::Child(const char **argv, int cin_, int cout_) : m_cin(-1), m_cout(-1) {
  int cin_pipe[2], cout_pipe[2], signal_pipe[2];
  pid_t p;

  if (cin_ == -1 && pipe(cin_pipe) < 0)
    goto fail1;

  if (cout_ == -1 && pipe(cout_pipe) < 0)
    goto fail2;

  if (pipe2(signal_pipe, O_CLOEXEC) < 0)
    goto fail3;

  fflush(stdout);

  p = fork();
  if (p < 0)
    goto fail4;

  if (p == 0) { // child

    close(signal_pipe[0]);

    int cin_fd;
    if (cin_ == -1) {
      close(cin_pipe[1]);
      cin_fd = cin_pipe[0];
    } else {
      cin_fd = cin_;
    }

    int cout_fd;
    if (cout_ == -1) {
      close(cout_pipe[0]);
      cout_fd = cout_pipe[1];
    } else {
      cout_fd = cout_;
    }

    if (cin_fd != STDIN_FILENO && dup2(cin_fd, STDIN_FILENO) < 0)
      goto child_fail;

    if (cout_fd != STDOUT_FILENO && dup2(cout_fd, STDOUT_FILENO) < 0)
      goto child_fail;

    execvp(argv[0], const_cast<char**>(argv));

child_fail:;
    int status = EXIT_FAILURE;
    ssize_t _ __attribute__((unused)) = write(signal_pipe[1], &status, sizeof status);
    _exit(status);

  }

  close(signal_pipe[1]);
  signal_pipe[1] = -1;

  int status;
  if (read(signal_pipe[0], &status, sizeof status) > 0) // child failed
    goto fail5;

  close(signal_pipe[0]);

  if (cin_ == -1) {
    close(cin_pipe[0]);
    m_cin = cin_pipe[1];
  }

  if (cout_ == -1) {
    close(cout_pipe[1]);
    m_cout = cout_pipe[0];
  }

  m_pid = p;

  return;

fail5:;
  int ignored;
  waitpid(p, &ignored, 0);
fail4:
  close(signal_pipe[0]);
  if (signal_pipe[1] >= 0)
    close(signal_pipe[1]);
fail3:
  if (cout_ == -1) {
    close(cout_pipe[0]);
    close(cout_pipe[1]);
  }
fail2:
  if (cin_ == -1) {
    close(cin_pipe[0]);
    close(cin_pipe[1]);
  }
fail1:
  m_pid = -1;
}

static int set_cloexec(int fd) {
  int flags = fcntl(fd, F_GETFD, 0);
  if (flags < 0)
    return flags;
  return fcntl(fd, F_SETFD, flags|FD_CLOEXEC);
}

int Child::cin_cloexec() {
  if (m_cin >= 0)
    return set_cloexec(m_cin);
  return 0;
}

int Child::cout_cloexec() {
  if (m_cout >= 0)
    return set_cloexec(m_cout);
  return 0;
}

int Child::kill() const {
  if (m_pid >= 0) {
    ::kill(m_pid, SIGTERM);
    return 0;
  }
  return -1;
}

int Child::wait() {
  if (m_pid >= 0) {
    int status;
    if (waitpid(m_pid, &status, 0) < 0)
      return -1;
    m_pid = -1;
    if (WIFEXITED(status))
      return WEXITSTATUS(status);
  }
  return -1;
}

void Child::hup() {
  if (m_cin >= 0)
    close(m_cin);
  m_cin = -1;
}

Child::~Child() {
  if (m_pid >= 0) {
    kill();
    wait();
  }
  if (m_cin >= 0)
    close(m_cin);
  if (m_cout >= 0)
    close(m_cout);
}

static Child *run_diff(int argc, char **argv) {
  static const char *argv_prefix[] = {
    "diff", "--show-c-function", "--unified", "--recursive", "--new-file" };
  static const size_t argv_prefix_len = sizeof argv_prefix / sizeof argv_prefix[0];

  const char **args = new (nothrow) const char*[argc + argv_prefix_len + 1];
  if (args == NULL)
    return NULL;

  for (unsigned i = 0; i < argv_prefix_len; i++)
    args[i] = argv_prefix[i];
  for (int i = 0; i < argc; i++)
    args[i + argv_prefix_len] = argv[i];
  args[argc + argv_prefix_len] = NULL;

  Child *c = new (nothrow) Child(args, STDIN_FILENO);

  delete[] args;

  return c;
}

namespace { class LineProcessor {

 public:
  virtual bool good() const = 0;
  virtual int process(const char *line) = 0;
  virtual ~LineProcessor() { }
  virtual void kill() { }
  virtual int wait() { return 0; }
  virtual void hup() { }

}; };

namespace { class Prettify : public LineProcessor {

  typedef enum {
    IDLE,
    LOOKAHEAD_ANSI_EXPR,
    COLOURISING,
    DROPPING_ANSI_EXPR,
  } state_t;

 public:
  explicit Prettify();
  virtual bool good() const override { return m_less != NULL; }
  virtual int process(const char *line) override;
  virtual void kill() override;
  virtual int wait() override;
  virtual void hup() override;
  ~Prettify();

 private:
  Child *m_dsf;
  Child *m_less;
  int m_cout;
  state_t state;

}; };

Prettify::Prettify() : m_dsf(NULL), m_less(NULL), state(IDLE) {

  int less_cin = -1;

  // See if we have diff-so-fancy, to further jazz up our diffs
  if (which("diff-so-fancy")) {
    const char *argv[] = { "diff-so-fancy", NULL };
    m_dsf = new (nothrow) Child(argv);
    if (m_dsf == NULL || !m_dsf->good()) {
      delete m_dsf;
      m_dsf = NULL;
      return;
    }
    int r __attribute__((unused)) = m_dsf->cin_cloexec();
    assert(r == 0);
    less_cin = m_dsf->cout();
    assert(less_cin != -1);
  }

  const char *argv[] = { "less", "--RAW-CONTROL-CHARS", "--quit-if-one-screen", "--no-init", "+Gg",
    NULL };
  m_less = new (nothrow) Child(argv, less_cin, STDOUT_FILENO);
  if (m_less == NULL) {
    delete m_dsf;
    m_dsf = NULL;
    return;
  }

  if (m_dsf != NULL) {
    m_cout = m_dsf->cin();
  } else {
    m_cout = m_less->cin();
  }
  assert(m_cout != -1);
}

static ssize_t write_string(int fd, const char *str) {
  return write(fd, str, strlen(str));
}

#define strprefix(s, s_lit) (strncmp((s), s_lit, sizeof s_lit - 1) == 0)

int Prettify::process(const char *line) {

  if (strprefix(line, "\033[33mcommit ")) {
    if (write_string(m_cout, line) < 0)
      return -1;
    return 0;
  }

  for (const char *p = line; *p; p++) {
    switch (state) {

      case IDLE:

        if (strprefix(p, "diff ") ||
            strprefix(p, "new file ") ||
            strprefix(p, "deleted file ") ||
            strprefix(p, "index ") ||
            strprefix(p, "--- ") ||
            strprefix(p, "+++ ")) {
          if (write_string(m_cout, "\033[1m") < 0)
            return -1;
          if (write(m_cout, p, 1) < 0)
            return -1;
          state = COLOURISING;
        } else if (strprefix(p, "@@ ")) {
          if (write_string(m_cout, "\033[36m") < 0)
            return -1;
          if (write(m_cout, p, 1) < 0)
            return -1;
          state = COLOURISING;
        } else {

          switch (*p) {

            case '+':
              if (write_string(m_cout, "\033[32m+") < 0)
                return -1;
              state = COLOURISING;
              break;

            case '-':
              if (write_string(m_cout, "\033[31m-") < 0)
                return -1;
              state = COLOURISING;
              break;

            case '\033':
              state = LOOKAHEAD_ANSI_EXPR;
              break;

            default:
              if (write_string(m_cout, p) < 0)
                return -1;
              return 0;

          }
        }
        break;

      case LOOKAHEAD_ANSI_EXPR:
        if (*p == 'm')
          state = IDLE;
        break;

      case COLOURISING:
        switch (*p) {

          case '\033':
            state = DROPPING_ANSI_EXPR;
            break;

          case '\n':
          case '\r':
            if (write_string(m_cout, "\033[0m") < 0)
              return -1;
            if (write(m_cout, p, 1) < 0)
              return -1;
            state = IDLE;
            break;

          default:
            if (write(m_cout, p, 1) < 0)
              return -1;

        }
        break;

      case DROPPING_ANSI_EXPR:
        if (*p == 'm')
          state = COLOURISING;
        break;

    }
  }
  return 0;
}

void Prettify::kill() {
  if (m_dsf != NULL)
    m_dsf->kill();
  m_less->kill();
}

int Prettify::wait() {
  int ret = 0;
  assert(good());
  hup();
  if (m_dsf != NULL) {
    ret |= m_dsf->wait();
    delete m_dsf;
    m_dsf = NULL;
  }

  ret |= m_less->wait();
  delete m_less;
  m_less = NULL;
  return ret;
}

void Prettify::hup() {
  if (m_dsf != NULL)
    m_dsf->hup();
  else
    m_less->hup();
}

Prettify::~Prettify() {
  delete m_dsf;
  delete m_less;
}

namespace { class Strip : public LineProcessor {

  typedef enum {
    IDLE,
    ANSI_EXPR,
  } state_t;

 public:
  explicit Strip() : state(IDLE) { }
  bool good() const override { return true; }
  virtual int process(const char *line) override;

 private:
  state_t state;

}; };

int Strip::process(const char *line) {
  for (const char *p = line; *p; p++) {
    if (state == IDLE) {
      if (*p == '\033')
        state = ANSI_EXPR;
      else
        fputc(*p, stdout);
    } else {
      assert(state == ANSI_EXPR);
      if (*p == 'm')
        state = IDLE;
    }
  }
  return 0;
}

int main(int argc, char **argv) {

  int in = STDIN_FILENO;

  Child *diff = NULL;
  if (argc > 1) {
    if (!which("diff")) {
      fprintf(stderr, "diff not found\n");
      return EXIT_FAILURE;
    }
    diff = run_diff(argc - 1, argv + 1);
    if (!diff->good()) {
      fprintf(stderr, "failed to run diff\n");
      return EXIT_FAILURE;
    }
    in = diff->cout();
  }

  FILE *fin = fdopen(in, "r");
  assert(fin != NULL);

  LineProcessor *processor;
  if (isatty(STDOUT_FILENO))
    processor = new (nothrow) Prettify();
  else
    processor = new (nothrow) Strip();
  if (processor == NULL || !processor->good()) {
    fprintf(stderr, "failed to start line processor\n");
    if (diff != NULL)
      delete diff;
    return EXIT_FAILURE;
  }

  int ret = EXIT_SUCCESS;

  char *line = NULL;
  size_t line_sz;
  for (;;) {

    ssize_t r = getline(&line, &line_sz, fin);
    if (r < 0) {
      free(line);
      if (feof(fin)) {
        break;
      }
      ret = EXIT_FAILURE;
      break;
    }

    if (processor->process(line) < 0) {
      fprintf(stderr, "failed to process line\n");
      free(line);
      ret = EXIT_FAILURE;
      break;
    }

  }

  if (diff != NULL) {
    diff->hup();
    if (ret == EXIT_FAILURE)
      diff->kill();
    diff->wait();
    delete diff;
  }

  if (ret == EXIT_FAILURE)
    processor->kill();
  processor->wait();
  delete processor;

  return ret;
}
