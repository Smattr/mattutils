#include <assert.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// An analysis pass.
struct analysis {
  int (*init)(struct analysis *this);
  int (*check)(struct analysis *this, const char *filename, unsigned lineno,
     const char *line);
  void (*deinit)(struct analysis *this);
  void *state;
};

static int undef_shift_init(struct analysis *this) {
  assert(this != NULL);

  regex_t *r = malloc(sizeof(*r));
  if (r == NULL)
    return -1;

  if (regcomp(r, "[[:alnum:]_][[:space:]]*<<[[:space:]]*([[:digit:]]+)", REG_EXTENDED) != 0) {
    free(r);
    return -1;
  }

  this->state = r;
  return 0;
}

static int undef_shift_check(struct analysis *this, const char *filename,
    unsigned lineno, const char *line) {
  assert(this != NULL);
  assert(this->state != NULL);

  regmatch_t match[2];
  static const size_t match_sz = sizeof(match) / sizeof(match[0]);
  if (regexec(this->state, line, match_sz, match, 0) == 0) {
    if (strtoul(line + match[1].rm_so, NULL, 10) >= 31) {
      fprintf(stderr,
        "%s:%u: %s\n"
        "potential undefined left shift\n", filename, lineno, line);
      return -1;
    }
  }

  return 0;
}

static void undef_shift_deinit(struct analysis *this) {
  assert(this != NULL);
  assert(this->state != NULL);
  regfree(this->state);
}

static int negate_self_init(struct analysis *this) {
  assert(this != NULL);

  regex_t *r = malloc(sizeof(*r));
  if (r == NULL)
    return -1;

  if (regcomp(r, "([[:alpha:]_][[:alnum:]]*)[[:space:]]*=[[:space:]]*-"
        "[[:space:]]*([[:alpha:]_][[:alnum:]]*)", REG_EXTENDED) != 0) {
    free(r);
    return -1;
  }

  this->state = r;
  return 0;
}

static int negate_self_check(struct analysis *this, const char *filename,
    unsigned lineno, const char *line) {
  assert(this != NULL);
  assert(this->state != NULL);

  regmatch_t match[3];
  static const size_t match_sz = sizeof(match) / sizeof(match[0]);
  if (regexec(this->state, line, match_sz, match, 0) == 0) {
    const char *s1 = line + match[1].rm_so;
    size_t len1 = match[1].rm_eo - match[1].rm_so;
    const char *s2 = line + match[2].rm_so;
    size_t len2 = match[2].rm_eo - match[2].rm_so;
    if (len1 == len2 && strncmp(s1, s2, len1) == 0) {
      fprintf(stderr, "%s:%u: %s\n potential negation of INT_MIN\n", filename,
        lineno, line);
      return -1;
    }
  }

  return 0;
}

static void negate_self_deinit(struct analysis *this) {
  assert(this != NULL);
  assert(this->state != NULL);
  regfree(this->state);
}

struct analysis ANALYSES[] = {
  {
    .init = undef_shift_init,
    .check = undef_shift_check,
    .deinit = undef_shift_deinit,
  },
  {
    .init = negate_self_init,
    .check = negate_self_check,
    .deinit = negate_self_deinit,
  },
};

static int scan_file(const char *path) {
  FILE *f = fopen(path, "r");
  if (f == NULL) {
    fprintf(stderr, "failed to open %s\n", path);
    return -1;
  }

  int ret = 0;

  char *line = NULL;
  size_t line_sz = 0;
  unsigned lineno = 0;
  for (;;) {
    lineno++;

    // Read a line from the file
    ssize_t r = getline(&line, &line_sz, f);
    if (r == -1) {
      if (!feof(f)) {
        fprintf(stderr, "failed to read from %s\n", path);
        ret = -1;
      }
      break;
    }

    // Strip trailing \n to make it easier for analyses
    if (line[strlen(line) - 1] == '\n')
      line[strlen(line) - 1] = '\0';

    // Run analysis passes
    for (size_t i = 0; i < sizeof(ANALYSES) / sizeof(ANALYSES[0]); i++)
      ret |= ANALYSES[i].check(&ANALYSES[i], path, lineno, line);
  }

  free(line);
  fclose(f);
  return ret;
}

static int analyse_path(const char *path) {
  struct stat buf;
  if (stat(path, &buf) < 0) {
    fprintf(stderr, "failed to stat %s; skipping\n", path);
    return -1;
  }

  if (S_ISDIR(buf.st_mode)) {
    // TODO: handle directories
    return -1;
  } else if (S_ISREG(buf.st_mode)) {
    return scan_file(path);
  } else {
    fprintf(stderr, "%s of unsupported type; skipping\n", path);
    return -1;
  }
}

static int analyse(const char **paths, size_t paths_len) {

  // Initialise all analysis passes
  for (size_t i = 0; i < sizeof(ANALYSES) / sizeof(ANALYSES[0]); i++) {
    if (ANALYSES[i].init(&ANALYSES[i]) != 0) {
      fprintf(stderr, "failed to initialise analysis %zu\n", i);
      for (size_t j = 0; j < i; j++)
        ANALYSES[j].deinit(&ANALYSES[j]);
      return -1;
    }
  }

  int ret = 0;

  for (size_t i = 0; i < paths_len; i++) {
    assert(paths[i] != NULL);
    ret |= analyse_path(paths[i]);
  }

  // Clean up analysis passes
  for (size_t i = 0; i < sizeof(ANALYSES) / sizeof(ANALYSES[0]); i++)
    ANALYSES[i].deinit(&ANALYSES[i]);

  return ret;
}

int main(int argc, char **argv) {

  if (argc > 1 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-?") == 0)) {
    fprintf(stderr, "usage: %s [paths...]\n"
                    "\n"
                    "C undefined behaviour linter\n", argv[0]);
    return EXIT_FAILURE;
  }

  if (argc == 1) {
    const char *paths[] = { "." };
    return analyse(paths, sizeof(paths) / sizeof(paths[0]));
  } else {
    return analyse((const char**)(argv + 1), (size_t)(argc - 1));
  }
}
