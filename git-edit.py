#!/usr/bin/env python3

"""
Automate rebasing of the current feature branch

A frequent operation when addressing code reviews is rewriting the commits on
the current branch while not advancing the point at which it branched off the
trunk. By not advancing the branch point, unaltered prefix commits remain the
same, allowing reviewers to more easily track which commits they have already
reviewed.
"""

import argparse
import re
import shlex
import subprocess as sp
import sys
from typing import List


def run(args: List[str]):
    print(f"+ {' '.join(shlex.quote(str(x)) for x in args)}")
    sp.check_call(args)


def call(args: List[str]):
    print(f"+ {' '.join(shlex.quote(str(x)) for x in args)}")
    return sp.check_output(args, universal_newlines=True).strip()


def main(args: List[str]) -> int:

    # parse command line options
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--onto", help="base branch of the current feature branch")
    options = parser.parse_args(args[1:])

    # check this is actually a Git repository
    run(["git", "rev-parse", "HEAD"])

    # if the user did not give us a base, figure it out from upstream
    if options.onto is None:
        show = call(["git", "remote", "show", "origin"])
        default = re.search(r"^\s*HEAD branch: (.*)$", show, flags=re.MULTILINE)
        if default is None:
            sys.stderr.write("could not figure out default branch name\n")
            return -1
        options.onto = default.group(1)

    # determine the point at which we branched off the base
    merge_base = call(["git", "merge-base", f"origin/{options.onto}", "HEAD"])

    # run a rebase, assuming we want to use `rerebase`
    run(["git", "re", "--interactive", merge_base])

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
