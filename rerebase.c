/* Rerebase - Git rebase with commit diffs along for the ride
 *
 * In a large interactive Git rebase, it is common to find yourself staring at short commit
 * messages, trying to recall what the contents of that commit actually were. I find myself often
 * suspending the picklist to run various `git show` commands to inspect commits. It's possible to
 * tweak rebase.intructionFormat to give you a slightly more informative line, but things can still
 * be quite cryptic. To ease this pain, this tool wraps Vim to act as your GIT_EDITOR. It retrieves
 * diffs for each commit in the picklist, appending them as folded comments to the commit line. So
 * instead of getting a picklist like:
 *
 *     pick 3897e6b reroute: Turbo charge functionality.
 *     pick bf6a3e5 Deprecate notme for reroute.
 *     pick 891c18c Remove deprecated magicln.
 *     ...
 *
 * you get one that reads like:
 *
 *     +--235 lines: pick 3897e6b reroute: Turbo charge functionality. ---------
 *     +--125 lines: pick bf6a3e5 Deprecate notme for reroute. -----------------
 *     +-- 60 lines: pick 891c18c Remove deprecated magicln. -------------------
 *     ...
 *
 * Unfolding any commit line gives you its associated diff:
 *
 *     +--235 lines: pick 3897e6b reroute: Turbo charge functionality. ---------
 *     pick bf6a3e5 Deprecate notme for reroute. {{{
 *      # commit bf6a3e5ec2e7258ccec67a179660d10fbb6d4f65
 *      # Author: Matthew Fernandez <matthew.fernandez@gmail.com>
 *      # Date:   Tue Jul 26 19:09:38 2016 +1000
 *      #
 *      #     Deprecate notme for reroute.
 *      #
 *      # diff --git a/config/.notme.json b/config/.notme.json
 *      # deleted file mode 100644
 *      # index 03e2a08..0000000
 *      # --- a/config/.notme.json
 *      # +++ /dev/null
 *      # @@ -1,4 +0,0 @@
 *      # -{
 *      # -    "cmake":["-G", "Ninja"],
 *      # -    "ag":["--group", "--color", "--pager", "less"]
 *      # -}
 *     ...
 *
 * To use this, you probably want to configure a Git alias for it. Something like:
 *
 *     [alias]
 *         re = "!f() { GIT_EDITOR=rerebase git rebase \"$@\"; }; f"
 *
 * Happy hacking.
 */

#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// `rename`, but works across devices
static int move(const char *oldpath, const char *newpath) {
    if (rename(oldpath, newpath) < 0) {
        if (errno == EXDEV) {
            int src = open(oldpath, O_RDONLY);
            if (src < 0)
                return -1;
            int dst = open(newpath, O_WRONLY|O_TRUNC|O_CREAT, 0644);
            if (dst < 0) {
                close(src);
                return -1;
            }
            struct stat st;
            if (fstat(src, &st) < 0) {
                close(dst);
                close(src);
                return -1;
            }
            int r = sendfile(dst, src, NULL, st.st_size);
            close(dst);
            close(src);
            if (r == 0)
                unlink(oldpath);
            return r;
        }
        return -1;
    }
    return 0;
}

// Insert diffs in-between lines in a rebase picklist
static int insert_diffs(FILE *in, FILE *out) {
    regex_t pick;
    bool regex_inited = false;
    int ret = 0;
    char *line = NULL;

    // we'll use this to look for lines representing commits in the input
    ret = regcomp(&pick, "^(pick|fixup|squash) ([[:xdigit:]]+) ", REG_EXTENDED);
    if (ret != 0)
        goto end;
    regex_inited = true;

    size_t line_sz;
    for (;;) {

        errno = 0;
        ssize_t r = getline(&line, &line_sz, in);
        if (r < 0) {
            ret = -errno;
            if (feof(in))
                break;
            goto end;
        }

        regmatch_t match[3];
        if (regexec(&pick, line, sizeof match / sizeof match[0], match, 0) == 0) {
            // this line is a commit

            assert(match[2].rm_so != -1);

            // Lop off the newline if there is one
            if (line[r - 1] == '\n')
                r--;

            // Write the line with a Vim marker to indicate the start of a fold
            if (fprintf(out, "%.*s {{{\n", (int)r, line) < 0) {
                ret = -1;
                goto end;
            }

            // Create a Git command to get a diff of this commit
            char *command;
            if (asprintf(&command, "git show %.*s", (int)(match[2].rm_eo - match[2].rm_so),
                    &line[match[2].rm_so]) < 0) {
                ret = -ENOMEM;
                goto end;
            }

            // Retrieve the diff and write it to the output
            errno = 0;
            FILE *pipe = popen(command, "r");
            if (pipe == NULL)
                ret = errno == 0 ? -ENOMEM : -errno;
            free(command);
            if (ret != 0)
                goto end;
            char *l = NULL;
            size_t l_sz;
            for (;;) {
                errno = 0;
                if (getline(&l, &l_sz, pipe) < 0) {
                    ret = -errno;
                    if (feof(pipe))
                        break;
                    free(l);
                    pclose(pipe);
                    goto end;
                }

                if (fprintf(out, " # %s", l) < 0) {
                    ret = -1;
                    free(l);
                    pclose(pipe);
                    goto end;
                }
            }
            free(l);
            int status = pclose(pipe);
            if (status == -1) {
                ret = -1;
                goto end;
            }
            if (WEXITSTATUS(status) != EXIT_SUCCESS) {
                ret = WEXITSTATUS(status);
                goto end;
            }

            // write a fold close marker
            if (fputs(" # }}}\n", out) < 0) {
                ret = -1;
                goto end;
            }

        } else {
            // normal line; write it out as-is
            size_t w = fwrite(line, 1, (size_t)r, out);
            if (w != (size_t)r) {
                ret = -1;
                goto end;
            }
        }
    }

end:
    free(line);
    if (regex_inited)
        regfree(&pick);
    return ret;
}

static int strip_comments(FILE *in, FILE *out) {
    char *line = NULL;
    size_t line_sz;
    int ret = 0;

    for (;;) {

        errno = 0;
        ssize_t r = getline(&line, &line_sz, in);
        if (r < 0) {
            ret = -errno;
            if (feof(in))
                break;
            goto end;
        }

        // Is this a comment we inserted?
        if (!strncmp(line, " #", sizeof " #" - 1))
            continue;

        size_t w = fwrite(line, 1, r, out);
        if (w != (size_t)r) {
            ret = -1;
            goto end;
        }
    }

end:
    free(line);
    return ret;
}

int main(int argc, char **argv) {
    FILE *in, *out;
    char *out_file;
    const char *tmp;

    if (argc != 2) {
        fprintf(stderr, "usage: %s rebase-file\n", argv[0]);
        return EXIT_FAILURE;
    }

    in = fopen(argv[1], "r");
    if (in == NULL) {
        perror("failed to open file");
        return EXIT_FAILURE;
    }

    // open a temporary file for making a new rebase input
    tmp = getenv("TMPDIR");
    if (tmp == NULL)
        tmp = "/tmp";
    if (asprintf(&out_file, "%s/tmp.XXXXXX", tmp) < 0) {
        fprintf(stderr, "failed to create temporary path\n");
        fclose(in);
        return EXIT_FAILURE;
    }
    {
        int out_fd = mkstemp(out_file);
        if (out_fd < 0) {
            perror("failed to open temporary file");
            free(out_file);
            fclose(in);
            return EXIT_FAILURE;
        }
        out = fdopen(out_fd, "w");
        assert(out != NULL);
    }

    {
        int ret = insert_diffs(in, out);
        fclose(out);
        fclose(in);
        if (ret != 0) {
            fprintf(stderr, "failed to insert diffs: %s\n", strerror(-ret));
            unlink(out_file);
            free(out_file);
            return EXIT_FAILURE;
        }
    }

    // Overwrite the original input file with our modified version
    {
        int ret = move(out_file, argv[1]);
        if (ret < 0)
            unlink(out_file);
        free(out_file);
        if (ret < 0) {
            fprintf(stderr, "failed to overwrite original input\n");
            return EXIT_FAILURE;
        }
    }

    // Now open Vim with the new file with all folds collapsed
    {
        char *command;
        if (asprintf(&command, "vim '+set foldmethod=marker' '+set foldlevel=0' '%s'", argv[1])
                < 0) {
            fprintf(stderr, "failed to construct Vim command\n");
            return EXIT_FAILURE;
        }
        int ret = system(command);
        free(command);
        if (ret == -1) {
            fprintf(stderr, "executing Vim failed\n");
            return EXIT_FAILURE;
        }
        if (WEXITSTATUS(ret) != EXIT_SUCCESS)
            return WEXITSTATUS(ret);
    }

    /* Now strip the comment lines. This is not strictly necessary, but Git's interactive rebase
     * script seems incredibly slow to pass over long stretches of comment lines.
     */

    in = fopen(argv[1], "r");
    if (in == NULL) {
        perror("failed to open file");
        return EXIT_FAILURE;
    }

    if (asprintf(&out_file, "%s/tmp.XXXXXX", tmp) < 0) {
        fprintf(stderr, "failed to create temporary path\n");
        fclose(in);
        return EXIT_FAILURE;
    }
    {
        int out_fd = mkstemp(out_file);
        if (out_fd < 0) {
            perror("failed to open temporary file");
            free(out_file);
            fclose(in);
            return EXIT_FAILURE;
        }
        out = fdopen(out_fd, "w");
        assert(out != NULL);
    }

    {
        int ret = strip_comments(in, out);
        fclose(out);
        fclose(in);
        if (ret != 0) {
            fprintf(stderr, "failed to strip comments: %s\n", strerror(-ret));
            unlink(out_file);
            free(out_file);
            return EXIT_FAILURE;
        }
    }

    {
        int ret = move(out_file, argv[1]);
        if (ret < 0)
            unlink(out_file);
        free(out_file);
        if (ret < 0) {
            fprintf(stderr, "failed to overwrite original input\n");
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
