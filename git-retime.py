#!/usr/bin/env python3

"""
rewrite Git timestamps
"""

import argparse
import os
import re
import shlex
import subprocess as sp
import sys
from typing import List


def run(args: List[str]):
    env = os.environ.copy()
    env["FILTER_BRANCH_SQUELCH_WARNING"] = "1"
    print(f"+ env FILTER_BRANCH_SQUELCH_WARNING=1 {shlex.join(str(x) for x in args)}")
    sp.check_call(args, env=env)


def call(args: List[str]):
    print(f"+ {shlex.join(str(x) for x in args)}")
    return sp.check_output(args, universal_newlines=True)


def main(args: List[str]) -> int:

    # parse command line options
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="preview what destructive actions would be run",
    )
    parser.add_argument(
        "--remote", help="remote to check for default branch", default="origin"
    )
    parser.add_argument("--onto", help="branch that the target will be merged into")
    parser.add_argument("ref", nargs="*", help="HEAD/range to retime")
    options = parser.parse_args(args[1:])

    if len(options.ref) > 2:
        sys.stderr.write("at most 2 ref arguments allowed\n")
        return -1

    # check this is actually a Git repository
    run(["git", "rev-parse", "HEAD"])

    # is the working directory clean?
    changes = call(["git", "status", "--short", "--ignore-submodules"])
    if re.search(r"^.[^\?]", changes, flags=re.MULTILINE) is not None:
        sys.stderr.write("changes in working directory; aborting\n")
        return -1

    # if the user did not give us a base, figure it out from upstream
    if options.onto is None and len(options.ref) < 2:
        show = call(["git", "remote", "show", options.remote])
        default = re.search(r"^\s*HEAD branch: (.*)$", show, flags=re.MULTILINE)
        if default is None:
            sys.stderr.write("could not figure out default branch name\n")
            return -1
        options.onto = f"{options.remote}/{default.group(1)}"

    # find the head to which we will retime
    if len(options.ref) >= 1:
        head = options.ref[-1]
    else:
        head = "HEAD"

    # find the base from which we will retime
    if len(options.ref) == 2:
        base = options.ref[0]
    else:
        base = call(["git", "merge-base", "--", options.onto, head]).strip()

    # get current timestamp in the right format
    now = call(["date", "--rfc-email"]).strip()

    cmd = [
        "git",
        "filter-branch",
        "--force",
        "--env-filter",
        f'export GIT_AUTHOR_DATE="{now}"\nexport GIT_COMMITTER_DATE="{now}"',
        "--",
        f"{base}..{head}",
    ]
    if options.dry_run:
        print(f"would run: {shlex.join(str(c) for c in cmd)}")
    else:
        run(cmd)

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
