#!/usr/bin/env python3

"""
Rebase the current branch onto the default upstream branch, bringing it up to
date.
"""

import argparse
import re
import shlex
import subprocess as sp
import sys
from typing import List


def run(*args: str):
    print(f"+ {shlex.join(str(x) for x in args)}")
    sp.check_call(args)


def call(*args: str):
    print(f"+ {shlex.join(str(x) for x in args)}")
    return sp.check_output(args, universal_newlines=True)


def main(args: List[str]) -> int:

    # parse command line options
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--onto", help="branch to rebase onto")
    parser.add_argument("--remote", help="remote to compare against", default="origin")
    options = parser.parse_args(args[1:])

    # check this is actually a Git repository
    run("git", "rev-parse", "HEAD")

    # if the user did not give us a base, figure it out from upstream
    if options.onto is None:
        show = call("git", "remote", "show", options.remote)
        default = re.search(r"^\s*HEAD branch: (.*)$", show, flags=re.MULTILINE)
        if default is None:
            sys.stderr.write("could not figure out default branch name\n")
            return -1
        options.onto = default.group(1)

    # run the rebase
    run("git", "pull", "--rebase", options.remote, options.onto)

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
