/* WIP of a tool to do exhaustive testing of git revisions. */

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <git2.h>
#include <iostream>
#include <json.hpp> // https://github.com/nlohmann/json
#include <fstream>
#include <map>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

using namespace std;

// C++ RAII bullshit. Pay no attention to that human behind the curtain.
class _manage {
  enum tag { OBJECT, REVWALK, REFERENCE };
  tag tag;
  union { git_object *object_p; git_revwalk *revwalk_p; git_reference *reference_p; };
 public:
  _manage(git_object *p): tag(OBJECT), object_p(p) {}
  _manage(git_revwalk *p): tag(REVWALK), revwalk_p(p) {}
  _manage(git_reference *p): tag(REFERENCE), reference_p(p) {}
  ~_manage() {
    switch (tag) {
      case OBJECT: if (object_p) git_object_free(object_p); break;
      case REVWALK: git_revwalk_free(revwalk_p); break;
      case REFERENCE: git_reference_free(reference_p); break;
    }
  }
};
#define _JOIN(x, y) x ## y
#define JOIN(x, y) _JOIN(x, y)
#define MANAGE(ptr) _manage JOIN(_, __COUNTER__)(ptr)
// End C++ RAII bullshit. Pay attention again.

// For convenience, to indicate when we're talking about a commit hash.
typedef string sha;

enum quality_t {
  GOOD,
  BAD,
  SKIPPED,
  UNTESTED,
};

static string to_string(quality_t q) {
  switch (q) {
    case GOOD: return "good";
    case BAD: return "bad";
    case SKIPPED: return "skip";
    case UNTESTED: return "untested";
  }
}

static quality_t from_string(const string &q) {
  if (q == "good") {
    return GOOD;
  } else if (q == "bad") {
    return BAD;
  } else if (q == "skip") {
    return SKIPPED;
  } else if (q == "untested") {
    return UNTESTED;
  } else {
    //TODO
    return UNTESTED;
  }
}

static sha to_sha(const git_oid *oid) {
  char commit[41];
  git_oid_fmt(commit, oid);
  commit[sizeof(commit) - 1] = '\0';
  return commit;
}

namespace { struct Result {
  sha commit;
  quality_t quality;
}; };

namespace { class State {

 public:
  int load(const string &path) {

    set_path(path);

    ifstream in(path);
    if (!in.is_open()) {
      cerr << "Failed to open " << path << ". No git-linear in progress?\n";
      return -1;
    }

    try {
      nlohmann::json j;
      in >> j;

      home = j["home"];

      for (auto item : j["progress"]) {
        Result r { item["commit"], from_string(item["quality"]) };
        commits.push_back(r);
      }

      log_text = j["log"];

    } catch (exception &e) {
      cerr << "Failed to parse " << path << ": " << e.what() << "\n";
      return -1;
    }

    return 0;
  }

  int save() {
    assert(path != "");

    nlohmann::json progress;
    for (const Result &r : commits) {
      progress.push_back(
        {{"commit", r.commit}, {"quality", to_string(r.quality)}});
    }

    nlohmann::json j {
      {"comment", "hello world"},
      {"home", home},
      {"progress", progress},
      {"log", log_text},
    };

    ofstream out(path);
    if (!out.is_open()) {
      cerr << "Failed to open " << path << ".\n";
      return -1;
    }

    out << setw(2) << j;

    return 0;
  }

  void set_path(const string &path) {
    this->path = path;
  }

  void mark(const sha &commit, quality_t quality) {
    for (Result &res : commits) {
      if (res.commit == commit) {
        res.quality = quality;
        break;
      }
    }
  }

  void enqueue(const sha &commit) {
    Result r { commit, UNTESTED };
    commits.push_back(r);
  }

  bool contains(const sha &commit) const {
    return find_if(begin(commits), end(commits), [commit](const Result r){
        return r.commit == commit;
      }) != end(commits);
  }

  bool done() const {
    return find_if(begin(commits), end(commits), [](const Result r){
        return r.quality == UNTESTED;
      }) == end(commits);
  }

  sha next() const {
    assert(!done());
    for (auto it = commits.rbegin(); it != commits.rend(); it++)
      if (it->quality == UNTESTED)
        return it->commit;
    assert(!"unreachable");
  }

  int remove() const {
    assert(path != "");
    return std::remove(path.c_str());
  }

  sha first, last;

  vector<Result> commits;
  
  sha home;
  string log_text;

  void log(int argc, char **argv) {
    string cmd("git-linear");
    for (int i = 0; i < argc; i++) {
      cmd += " '";
      for (const char *p = argv[i]; *p != '\0'; p++) {
        if (*p == '\'')
          cmd += "''";
        else
          cmd += *p;
      }
      cmd += "'";
    }
    log_text += cmd + "\n";
  }

 private:
  string path;

}; };

static int checkout(git_repository *repo, const sha &commit) {

  git_object *obj;
  if (git_revparse_single(&obj, repo, commit.c_str()) < 0) {
    /* In theory this should never happen, but maybe the user has done something
     * dumb like messed with the SHAs in .git-linear.json or relocated
     * .git-linear.json to an unrelated repo.
     */
    const git_error *e = giterr_last();
    cerr << "Could not parse SHA " << commit << ": " << e->message << "\n";
    return EXIT_FAILURE;
  }
  MANAGE(obj);

  /* `git-checkout <obj>`. The way to achieve this in libgit2 is surprisingly
   * unintuitive.
   */

  // Now checkout the working directory to HEAD.
#pragma clang diagnostic push // sigh
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
  git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
#pragma clang diagnostic pop
  opts.checkout_strategy = GIT_CHECKOUT_SAFE;
  if (git_checkout_tree(repo, obj, &opts) < 0) {
    const git_error *e = giterr_last();
    cerr << "Failed to checkout tree: " << e->message << "\n";
    return EXIT_FAILURE;
  }

  // First try to set an attached HEAD.
  if (git_repository_set_head(repo, commit.c_str()) < 0) {
    // We failed; set a detached one.
    if (git_repository_set_head_detached(repo, git_object_id(obj)) < 0) {
      const git_error *e = giterr_last();
      cerr << "Failed to set HEAD: " << e->message << "\n";
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

static int checkout_next(git_repository *repo, State &state) {

  // Sync our current state to disk before we continue.
  if (state.save() < 0) {
    cerr << "Failed to save current state.\n";
    return EXIT_FAILURE;
  }

  // Abort if there's nothing left to check.
  if (state.done()) {
    cout << "git-linear complete. Run `git-linear status` to see results.\n";
    return EXIT_SUCCESS;
  }

  sha commit = state.next();

  return checkout(repo, commit);
}

static int action_add(git_repository *repo, State &state, int argc,
    char **argv, bool no_log = false) {

  if (argc < 2 || argc > 3) {
    cerr << "expected rev-spec\n"
            "run `git-linear help` for usage\n";
    return EXIT_FAILURE;
  }

  /* Unpack the spec description the user has given us to a canonical
   * representation.
   */
  git_object *from = nullptr, *to = nullptr;
  MANAGE(from);
  MANAGE(to);
  if (argc == 2) {

    string spec(argv[1]);

    if (spec.find("..") == string::npos) {

      // This is not a range; this is a single commit.
      git_object *obj;
      if (git_revparse_single(&obj, repo, spec.c_str()) < 0) {
        const git_error *e = giterr_last();
        cerr << "failed to parse revspec \"" << argv[1] << "\": " <<
          e->message << "\n";
        return EXIT_FAILURE;
      }
      to = obj;

    } else {

      /* It's possible the user gave us an open range (e.g. "..master") in which
       * case we need to substitute HEAD for the missing limit.
       */
      if (spec.size() > 2 && spec.substr(0, 2) == "..") {
        spec = "HEAD" + spec;
      } else if (spec.size() > 2 && spec.substr(spec.size() - 2, 2) == "..") {
        spec += "HEAD";
      }

      git_revspec revspec;
      if (git_revparse(&revspec, repo, spec.c_str()) < 0) {
        const git_error *e = giterr_last();
        cerr << "failed to parse revspec \"" << argv[1] << "\": " <<
          e->message << "\n";
        return EXIT_FAILURE;
      }
      from = revspec.from;
      to = revspec.to;

    }

  } else {
    assert(argc == 3);

    if (git_revparse_single(&from, repo, argv[1]) < 0) {
      const git_error *e = giterr_last();
      cerr << "failed to parse revspec \"" << argv[1] << "\": " << e->message <<
        "\n";
      return EXIT_FAILURE;
    }

    if (git_revparse_single(&to, repo, argv[2]) < 0) {
      const git_error *e = giterr_last();
      cerr << "failed to parse revspec \"" << argv[2] << "\": " << e->message <<
        "\n";
      return EXIT_FAILURE;
    }
  }

  git_revwalk *walk;
  if (git_revwalk_new(&walk, repo) < 0) {
    const git_error *e = giterr_last();
    cerr << "failed to create a revwalk: " << e->message << "\n";
    return EXIT_FAILURE;
  }
  MANAGE(walk);

  if (from != nullptr) {
    if (git_revwalk_push(walk, git_object_id(to)) < 0) {
      const git_error *e = giterr_last();
      cerr << "failed to push revwalk end: " << e->message << "\n";
      return EXIT_FAILURE;
    }
    if (git_revwalk_hide(walk, git_object_id(from)) < 0) {
      const git_error *e = giterr_last();
      cerr << "failed to push revwalk start: " << e->message << "\n";
      return EXIT_FAILURE;
    }

    // Do the walk itself and note each commit we see.
    git_oid oid;
    while (git_revwalk_next(&oid, walk) == 0)
      state.enqueue(to_sha(&oid));
  } else {
    state.enqueue(to_sha(git_object_id(to)));
  }

  if (state.home == "") {
    /* We are just starting a git-linear and need to set a home to later return
     * to.
     */

    /* Get a reference to HEAD, so we can figure out what it is. Despite
     * git_repository_head looking promising and numerous sources online point
     * to it, it is not the droid we are looking for. It fully resolves the
     * symbolic link chain, meaning we can never observe whether HEAD first
     * points to a branch.
     */
    git_reference *ref;
    if (git_reference_lookup(&ref, repo, "HEAD") < 0) {
      const git_error *e = giterr_last();
      cerr << "Failed to retrieve a reference for HEAD: " << e->message << "\n";
      return EXIT_FAILURE;
    }
    MANAGE(ref);

    // See if HEAD points at a branch or a random commit.
    git_ref_t type = git_reference_type(ref);
    assert(type == GIT_REF_OID || type == GIT_REF_SYMBOLIC);

    if (type == GIT_REF_SYMBOLIC) {
      /* HEAD points at a branch. We want to store the branch name, not its
       * commit SHA as the home. If we store its SHA, when we `git-linear reset`
       * we end up detached.
       */
      const char *branch = git_reference_symbolic_target(ref);
      if (branch == nullptr) {
        // unreachable?
        cerr << "HEAD is a symbolic reference, but its target has no name\n";
        return EXIT_FAILURE;
      }

      state.home = branch;

    } else {
      // HEAD points at a random commit. Store its SHA.
      assert(type == GIT_REF_OID);

      git_oid oid;
      if (git_reference_name_to_id(&oid, repo, "HEAD") < 0) {
        // unreachable?
        const git_error *e = giterr_last();
        cerr << "Failed to get ID of HEAD: " << e->message << "\n";
        return EXIT_FAILURE;
      }

      state.home = to_sha(&oid);
    }

  }

  if (!no_log)
    state.log(argc, argv);

  return checkout_next(repo, state);
}

static int action_start(git_repository *repo, const string &config, int argc,
    char **argv) {

  if (argc < 2 || argc > 3) {
    cerr << "expected rev-spec\n"
            "run `git-linear help` for usage\n";
    return EXIT_FAILURE;
  }

  if (access(config.c_str(), R_OK) == 0) {
    cerr << config << " exists; git-linear in progress?\n"
      "run `git-linear reset` or remove " << config << " to abort an "
        "in-progress git-linear\n";
    return EXIT_FAILURE;
  }

  // We're starting from scratch. Create a new state.
  State state;
  state.set_path(config);

  state.log(argc, argv);

  // XXX: We know the callee won't look at argv[0]
  return action_add(repo, state, argc, argv, true);
}

static int action_mark(git_repository *repo, State &state, int argc,
    char **argv) {

  assert(argc >= 1);
  quality_t quality;
  if (!strcmp(argv[0], "good")) {
    quality = GOOD;
  } else if (!strcmp(argv[0], "bad")) {
    quality = BAD;
  } else {
    assert(!strcmp(argv[0], "skip"));
    quality = SKIPPED;
  }

  if (argc > 2) {
    cerr << "Unrecognised arguments. Run `git-linear help` for usage.\n";
    return EXIT_FAILURE;
  }

  sha commit;
  if (argc == 2) {

    commit = argv[1];
    if (!state.contains(commit)) {
      cerr << "Commit " << commit << " does not lie in search range.\n";
      return EXIT_FAILURE;
    }

  } else {
    // Figure out what current commit we're looking at.
    git_object *obj;
    if (git_revparse_single(&obj, repo, "HEAD") < 0) {
      const git_error *e = giterr_last();
      cerr << "Failed to retrieve current HEAD: " << e->message << "\n";
      return EXIT_FAILURE;
    }
    MANAGE(obj);

    const git_oid *oid = git_object_id(obj);
    commit = to_sha(oid);
    if (!state.contains(commit)) {
      cerr << "The current commit is not in the search space.\n";
      return EXIT_FAILURE;
    }
  }

  state.mark(commit, quality);

  state.log(argc, argv);

  return checkout_next(repo, state);
}

static string first_line(const string &text) {
  auto it = find(text.begin(), text.end(), '\n');
  return string(text.begin(), it);
}

static int action_status(git_repository *repo, State &state, int argc,
    [[gnu::unused]] char **argv) {

  if (argc > 1) {
    cerr << "Unrecognised arguments. Run `git-linear help` for usage.\n";
    return EXIT_FAILURE;
  }

  bool tty = isatty(STDOUT_FILENO);
  const char *green = tty ? "\033[32m" : "";
  const char *yellow = tty ? "\033[33m" : "";
  const char *red = tty ? "\033[31m" : "";
  const char *reset = tty ? "\033[0m" : "";

  unsigned untested = 0;
  for (const Result &r : state.commits) {

    switch (r.quality) {
      case GOOD:
        cout << green << "good    " << reset;
        break;
      case BAD:
        cout << red << "bad     " << reset;
        break;
      case SKIPPED:
        cout << yellow << "skipped " << reset;
        break;
      case UNTESTED:
        cout << "untested";
        untested++;
        break;
    }

    cout << " " << r.commit;

    git_object *obj;
    string message;
    if (git_revparse_single(&obj, repo, r.commit.c_str()) < 0) {
      message = "<failed to retrieve commit message>";
    } else {
      message = first_line(git_commit_message((git_commit*)obj));
      MANAGE(obj);
    }

    cout << " " << message << "\n";
  }
  cout << untested << " commits remaining to test\n";
  return EXIT_SUCCESS;
}

static int action_reset(git_repository *repo, State &state, int argc,
    [[gnu::unused]] char **argv) {

  if (argc > 1) {
    cerr << "Unrecognised arguments. Run `git-linear help` for usage.\n";
    return EXIT_FAILURE;
  }

  // Delete on-disk state.
  if (state.remove() < 0) {
    cerr << "Failed to remove state: " << strerror(errno) << "\n";
    return EXIT_FAILURE;
  }

  // Return to where we started this git-linear.
  return checkout(repo, state.home);
}

static int action_run(git_repository *repo, State &state, int argc,
    char **argv) {

  if (argc < 2) {
    cerr << "Unrecognised arguments. Run `git-linear help` for usage.\n";
    return EXIT_FAILURE;
  }

  /* Assume we are already within the git-linear range (i.e. do not checkout the
   * first pending commit here). Essentially we assume that if we're not on the
   * first commit, there's a reason the user wanted to start here.
   */

  while (!state.done()) {

    sha commit = state.next();

    /* Setup a file descriptor by which the child can indicate a failed exec to
     * us. If we ever get anything back on this file descriptor, we know the
     * exec failed.
     */
    int sig[2];
    if (pipe2(sig, O_CLOEXEC) < 0) {
      cerr << "Failed to create signal pipe: " << strerror(errno) << "\n";
      return EXIT_FAILURE;
    }

    pid_t pid = fork();
    if (pid < 0) {
      cerr << "Fork failed: " << strerror(errno) << "\n";
      return EXIT_FAILURE;
    }

    if (pid == 0) { // child

      // Set an environment variable for the script to see the current commit.
      if (setenv("GIT_COMMIT", commit.c_str(), 1) < 0) {
        cerr << "Failed to set GIT_COMMIT environment variable: " <<
          strerror(errno) << "\n";
        goto fail;
      }

      execvp(argv[1], argv + 1);

fail:
      char c = 1;
      write(sig[1], &c, sizeof(c));
      exit(EXIT_FAILURE);

    }

    /* Close the end of the pipe we (the parent) don't need. We need to do this
     * to ensure our attempted read is interrupted when our child execs.
     */
    close(sig[1]);

    /* Try to read from the pipe. If the child correctly execs, this should
     * fail.
     */
    char dummy;
    if (read(sig[0], &dummy, sizeof(dummy)) > 0) {
      cerr << "Failed to exec command\n";
      return EXIT_FAILURE;
    }

    int status;
    if (waitpid(pid, &status, 0) < 0) {
      cerr << "Failed to wait on child\n";
      return EXIT_FAILURE;
    }

    /* Determine how to mark this commit, using the same methodology as
     * git-bisect.
     */
    const char *quality;
    switch (WEXITSTATUS(status)) {

      case 0:
        quality = "good";
        break;

      case 1 ... 124:
      case 126 ... 127:
        quality = "bad";
        break;

      case 125:
        quality = "skip";
        break;

      default:
        cerr << "Command returned status outside [0, 127]; aborting\n";
        return EXIT_FAILURE;

    }

    const char *mark_argv[] = { quality, nullptr };
    int r = action_mark(repo, state, 1, const_cast<char**>(mark_argv));
    if (r != EXIT_SUCCESS)
      return r;

  }

  const char *status_argv[] = { "status", nullptr };
  return action_status(repo, state, 1, const_cast<char**>(status_argv));
}

static int action_log(State &state, int argc, [[gnu::unused]] char **argv) {

  if (argc > 1) {
    cerr << "Unrecognised arguments. Run `git-linear help` for usage.\n";
    return EXIT_FAILURE;
  }

  cout << state.log_text;

  return EXIT_SUCCESS;
}

static int action_replay(int argc, char **argv) {

  if (argc != 2) {
    cerr << "Unrecognised arguments. Run `git-linear help` for usage.\n";
    return EXIT_FAILURE;
  }

  ifstream in(argv[1]);
  if (!in.is_open()) {
    cerr << "Failed to open " << argv[1] << "\n";
    return EXIT_FAILURE;
  }

  /* This (intentionally) allows replay of arbitrary commands. */
  for (;;) {

    string line;
    getline(in, line);
    if (in.eof() || in.bad() || in.fail())
      break;

    while (line.size() > 0 && isspace(line[0]))
      line = line.substr(1, line.size() - 1);

    if (line == "" || line[0] == '#')
      continue;

    int r = system(line.c_str());
    if (r != 0)
      return r;
  }

  return EXIT_SUCCESS;
}

int main(int argc, char **argv) {

  if (argc < 2 || !strcmp(argv[1], "help")) {
    cout << "Test a range of commits in the manner of git-bisect, but exhaustively.\n\n"
      "usage:\n"
      " " << argv[0] << " start (<revspec>|<rev> <rev>) - start testing the range rev1..rev2\n"
      " " << argv[0] << " good [<rev>]                  - mark current commit as good\n"
      " " << argv[0] << " bad [<rev>]                   - mark current commit as bad\n"
      " " << argv[0] << " skip [<rev>]                  - skip current commit\n"
      " " << argv[0] << " status                        - show current progress\n"
      " " << argv[0] << " add <rev>                     - append some more commits to an in-progress scan\n"
      " " << argv[0] << " reset                         - abort testing and clean up metadata\n"
      " " << argv[0] << " run <cmd>...                  - automate the remaining testing using the given command\n"
      " " << argv[0] << " log                           - generate a log of actions\n"
      " " << argv[0] << " replay <file>                 - replay a previously generated log\n"
      " " << argv[0] << " help                          - show this information\n";
    return EXIT_FAILURE;
  }

  // Guard against careless refactoring.
  assert(argc > 1);

  if (!strcmp(argv[1], "replay"))
    return action_replay(argc - 1, argv + 1);

  git_libgit2_init();

  git_repository *repo;
  if (git_repository_open_ext(&repo, ".", 0, nullptr) < 0) {
    const git_error *e = giterr_last();
    cerr << "failed to open repository: " << e->message << "\n";
    git_libgit2_shutdown();
    return EXIT_FAILURE;
  }

  // Figure out where out state save area is.
  const char *wd = git_repository_workdir(repo);
  assert(wd != nullptr);
  string state_json(wd);
  state_json += ".git-linear.json";

  int ret;

  if (!strcmp(argv[1], "start")) {
    ret = action_start(repo, state_json, argc - 1, argv + 1);
  } else {

    // Load the previous state.
    State state;
    if (state.load(state_json) < 0) {
      git_libgit2_shutdown();
      return EXIT_FAILURE;
    }

    if (!strcmp(argv[1], "good") || !strcmp(argv[1], "bad") ||
        !strcmp(argv[1], "skip")) {
      ret = action_mark(repo, state, argc - 1, argv + 1);
    } else if (!strcmp(argv[1], "status")) {
      ret = action_status(repo, state, argc - 1, argv + 1);
    } else if (argc >= 2 && !strcmp(argv[1], "add")) {
      ret = action_add(repo, state, argc - 1, argv + 1);
    } else if (!strcmp(argv[1], "reset")) {
      ret = action_reset(repo, state, argc - 1, argv + 1);
    } else if (!strcmp(argv[1], "run")) {
      ret = action_run(repo, state, argc - 1, argv + 1);
    } else if (!strcmp(argv[1], "log")) {
      ret = action_log(state, argc - 1, argv + 1);
    }

    else {
      cerr << "unrecognised command \"" << argv[1] << "\"\n"
        "run \"" << argv[0] << " help\" for usage\n";
      ret = EXIT_FAILURE;
    }
  }

  git_libgit2_shutdown();

  return ret;
}
