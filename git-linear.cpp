/* WIP of a tool to do exhaustive testing of git revisions. */

#include <algorithm>
#include <cassert>
#include <cstring>
#include <git2.h>
#include <iostream>
#include <json.hpp> // https://github.com/nlohmann/json
#include <fstream>
#include <map>
#include <string>
#include <unistd.h>
#include <vector>

using namespace std;

// C++ RAII bullshit. Pay no attention to that human behind the curtain.
class _manage {
  enum tag { OBJECT, REVWALK };
  tag tag;
  union { git_object *object_p; git_revwalk *revwalk_p; };
 public:
  _manage(git_object *p): tag(OBJECT), object_p(p) {}
  _manage(git_revwalk *p): tag(REVWALK), revwalk_p(p) {}
  ~_manage() {
    switch (tag) {
      case OBJECT: if (object_p) git_object_free(object_p); break;
      case REVWALK: git_revwalk_free(revwalk_p); break;
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
      cerr << "Failed to open " << path << ".\n";
      return -1;
    }

    nlohmann::json j;
    in >> j;

    for (auto item : j["progress"]) {
      Result r { item["commit"], from_string(item["quality"]) };
      commits.push_back(r);
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
      {"progress", progress},
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

  sha first, last;
 private:
  string path;
  sha home;

  vector<Result> commits;
  
}; };


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

  if (git_repository_set_head_detached(repo, git_object_id(obj)) < 0) {
    const git_error *e = giterr_last();
    cerr << "Failed to set HEAD: " << e->message << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

static int action_start(git_repository *repo, const string &config, int argc, char **argv) {
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

  /* Unpack the spec description the user has given us to a canonical
   * representation.
   */
  git_object *from = nullptr, *to = nullptr;
  MANAGE(from);
  MANAGE(to);
  if (argc == 2) {

    /* It's possible the user gave us an open range (e.g. "..master") in which
     * case we need to substitute HEAD for the missing limit.
     */
    string spec(argv[1]);
    if (spec.size() > 2 && spec.substr(0, 2) == "..") {
      spec = "HEAD" + spec;
    } else if (spec.size() > 2 && spec.substr(spec.size() - 2, 2) == "..") {
      spec += "HEAD";
    }

    git_revspec revspec;
    if (git_revparse(&revspec, repo, spec.c_str()) < 0) {
      const git_error *e = giterr_last();
      cerr << "failed to parse revspec \"" << argv[1] << "\": " << e->message <<
        "\n";
      return EXIT_FAILURE;
    }
    from = revspec.from;
    to = revspec.to;

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

  // We're starting from scratch. Create a new state.
  State state;

  git_revwalk *walk;
  if (git_revwalk_new(&walk, repo) < 0) {
    const git_error *e = giterr_last();
    cerr << "failed to create a revwalk: " << e->message << "\n";
    return EXIT_FAILURE;
  }
  MANAGE(walk);

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

  state.set_path(config);

  return checkout_next(repo, state);
}

static int action_mark(git_repository *repo, State &state, int argc,
    char **argv) {

  assert(argc == 1 || argc == 2);
  quality_t quality;
  if (!strcmp(argv[0], "good")) {
    quality = GOOD;
  } else if (!strcmp(argv[0], "bad")) {
    quality = BAD;
  } else {
    assert(!strcmp(argv[0], "skip"));
    quality = SKIPPED;
  }

  // Figure out what current commit we're looking at.
  git_object *obj;
  if (git_revparse_single(&obj, repo, "HEAD") < 0) {
    const git_error *e = giterr_last();
    cerr << "Failed to retrieve current HEAD: " << e->message << "\n";
    return EXIT_FAILURE;
  }
  MANAGE(obj);

  const git_oid *oid = git_object_id(obj);
  sha commit = to_sha(oid);
  if (!state.contains(commit)) {
    cerr << "The current commit is not in the search space.\n";
    return EXIT_FAILURE;
  }

  state.mark(commit, quality);

  return checkout_next(repo, state);
}

int main(int argc, char **argv) {

  if (argc < 2 || !strcmp(argv[1], "help")) {
    cout << "Test a range of commits in the manner of git-bisect, but exhaustively.\n\n"
      "usage:\n"
      " " << argv[0] << " start <rev>          - start testing the range rev1..rev2\n"
      " " << argv[0] << " good [<rev>]         - mark current commit as good\n"
      " " << argv[0] << " bad [<rev>]          - mark current commit as bad\n"
      " " << argv[0] << " skip [<rev>]         - skip current commit\n"
#if 0
      " " << argv[0] << " run <cmd>...         - automate the remaining testing using the given command\n"
      " " << argv[0] << " add <rev>            - append some more commits to an in-progress scan\n"
      " " << argv[0] << " status               - show current progress\n"
      " " << argv[0] << " reset                - abort testing and clean up metadata\n"
      " " << argv[0] << " log                  - generate a log of actions\n"
      " " << argv[0] << " replay <file>        - replay a previously generated log\n"
#endif
      " " << argv[0] << " help                 - show this information\n";
    return EXIT_FAILURE;
  }

  // Guard against careless refactoring.
  assert(argc > 1);

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
      cerr << "Failed to read " << state_json << ". No git-linear in "
        "progress?\n";
      git_libgit2_shutdown();
      return EXIT_FAILURE;
    }

    if (argc == 2 && (!strcmp(argv[1], "good") || !strcmp(argv[1], "bad") ||
        !strcmp(argv[1], "skip"))) {
      ret = action_mark(repo, state, argc - 1, argv + 1);
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
