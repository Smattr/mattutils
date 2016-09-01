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
            int dst = open(newpath, O_WRONLY);
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

int main(int argc, char **argv) {
    FILE *in, *out;
    char *out_file, *line = NULL;
    size_t line_sz;

    if (argc != 2) {
        fprintf(stderr, "usage: %s rebase-file\n", argv[0]);
        goto fail;
    }

    in = fopen(argv[1], "r");
    if (in == NULL) {
        perror("failed to open file");
        goto fail;
    }

    // open a temporary file for making a new rebase input
    {
        const char *tmp = getenv("TMPDIR");
        if (tmp == NULL)
            tmp = "/tmp";
        if (asprintf(&out_file, "%s/tmp.XXXXXX", tmp) < 0) {
            fprintf(stderr, "failed to create temporary path\n");
            goto fail1;
        }
    }
    {
        int out_fd = mkstemp(out_file);
        if (out_fd < 0) {
            perror("failed to open temporary file");
            goto fail2;
        }
        out = fdopen(out_fd, "w");
        assert(out != NULL);
    }

    // we'll use this to look for lines representing commits in the input
    regex_t pick;
    if (regcomp(&pick, "^(pick|fixup|squash) ([[:xdigit:]]+) ", REG_EXTENDED) < 0) {
        fprintf(stderr, "failed to compile regular expression\n");
        goto fail3;
    }

    for (;;) {

        ssize_t r = getline(&line, &line_sz, in);
        if (r < 0) {
            int err = errno;
            if (feof(in))
                break;
            fprintf(stderr, "failed to read from input file: %s", strerror(err));
            goto fail4;
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
                fprintf(stderr, "failed to write to output file\n");
                goto fail4;
            }

            // Create a Git command to get a diff of this commit
            char *command;
            if (asprintf(&command, "git show %.*s", match[2].rm_eo - match[2].rm_so,
                    &line[match[2].rm_so]) < 0) {
                fprintf(stderr, "asprintf failed\n");
                goto fail4;
            }

            // Retrieve the diff and write it to the output
            FILE *pipe = popen(command, "r");
            free(command);
            if (pipe == NULL) {
                fprintf(stderr, "failed to open Git pipe\n");
                goto fail4;
            }
            char *l = NULL;
            size_t l_sz;
            for (;;) {
                ssize_t r2 = getline(&l, &l_sz, pipe);
                if (r2 < 0) {
                    int err = errno;
                    if (feof(pipe))
                        break;
                    fprintf(stderr, "failed to read from Git pipe: %s\n", strerror(err));
                    free(l);
                    pclose(pipe);
                    goto fail4;
                }

                if (fprintf(out, " # %s", l) < 0) {
                    fprintf(stderr, "failed to write to output file\n");
                    free(l);
                    pclose(pipe);
                    goto fail4;
                }
            }
            free(l);
            int status = pclose(pipe);
            if (status < 0 || WEXITSTATUS(status) != EXIT_SUCCESS) {
                fprintf(stderr, "git show failed\n");
                goto fail4;
            }

            // write a fold close marker
            if (fputs(" # }}}\n", out) < 0) {
                fprintf(stderr, "failed to write to output file\n");
                goto fail4;
            }

        } else {
            // normal line; write it out as-is
            size_t w = fwrite(line, 1, (size_t)r, out);
            if (w != (size_t)r) {
                fprintf(stderr, "failed to write to output file\n");
                goto fail4;
            }
        }

        continue;
fail4:
        free(line);
        regfree(&pick);
        goto fail3;

    }
    free(line);
    regfree(&pick);

    // Overwrite the original input file with our modified version
    fclose(out);
    fclose(in);
    if (move(out_file, argv[1]) < 0) {
        fprintf(stderr, "failed to overwrite original input\n");
        unlink(out_file);
        free(out_file);
        return EXIT_FAILURE;
    }
    free(out_file);

    // Now open Vim with the new file with all folds collapsed
    char *args[] = { "vim", "+set foldmethod=marker", "+set foldlevel=0", argv[1], NULL };
    execvp("vim", args);
    perror("failed exec");
    return EXIT_FAILURE;

fail3:
    fclose(out);
    unlink(out_file);
fail2:
    free(out_file);
fail1:
    fclose(in);
fail:
    return EXIT_FAILURE;
}
