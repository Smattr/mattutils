#!/usr/bin/env python3

"""
Git branch reaper

When developing in a branch that is reviewed and frequently rebased, a nice
scheme to use is to -2, -3, … suffix success versions of the rebased branch so
you can always go back to a prior series. This is nice during review, but leaves
you with numerous irrelevant branches after finally landing the changes.

This script automates the process of (1) checking the final version of this
branch is really merged into the default branch and (2) deleting it and all its
predecessors.
"""

import argparse
import re
import shlex
import subprocess as sp
import sys
from typing import List
import uuid

def run(args: List[str]):
  print(f"+ {' '.join(shlex.quote(str(x)) for x in args)}")
  sp.check_call(args)

def call(args: List[str]):
  print(f"+ {' '.join(shlex.quote(str(x)) for x in args)}")
  return sp.check_output(args, universal_newlines=True)

def force_delete(branch: str):
  run(["git", "branch", "--delete", "--force", branch])

def delete(branch: str, upstream: str):

  # move to the branch to delete
  run(["git", "checkout", branch])

  # create a new temporary branch
  tmp = str(uuid.uuid4())
  run(["git", "checkout", "-b", tmp])

  # rebase onto main
  run(["git", "pull", "--rebase", "origin", upstream])

  # check where this moved our pointer to
  us = call(["git", "rev-parse", tmp])
  them = call(["git", "rev-parse", f"origin/{upstream}"])

  # move to main
  run(["git", "checkout", upstream])

  # remove our temporary branch
  force_delete(tmp)

  # if rebasing left no commits in addition to main, we can delete the target
  if us == them:
    force_delete(branch)

  else:
    raise RuntimeError(
      f"{branch} still contained commits after rebasing {upstream}")

def main(args: List[str]) -> int:

  # parse command line options
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument("--onto", help="branch that the target was merged into")
  parser.add_argument("branch", help="branch to reap")
  options = parser.parse_args(args[1:])

  # check this is actually a Git repository
  run(["git", "rev-parse", "HEAD"])

  # is the working directory clean?
  changes = call(["git", "status", "--short", "--ignore-submodules"])
  if re.search(r"^.[^\?]", changes, flags=re.MULTILINE) is not None:
    sys.stderr.write("changes in working directory; aborting\n")
    return -1

  # if the user did not give us a base, figure it out from upstream
  if options.onto is None:
    show = call(["git", "remote", "show", "origin"])
    default = re.search(r"^\s*HEAD branch: (.*)$", show, flags=re.MULTILINE)
    if default is None:
      sys.stderr.write("could not figure out default branch name\n")
      return -1
    options.onto = default.group(1)

  # find the branch(es) we are aiming to remove
  victims: List[str] = []
  branches = call(["git", "branch"])
  for line in branches.split("\n"):
    branch = line[2:]
    if branch.startswith(options.branch):
      suffix = branch[len(options.branch):]
      if re.match(r"(-\d+)?$", suffix) is not None:
        victims.append(branch)

  print(f"going to delete {victims}")

  if len(victims) == 0:
    sys.stderr.write("no branches to delete\n")
    return -1

  # delete the branches is reverse order
  for i, branch in enumerate(reversed(victims)):

    # assume all non-final branches will not rebase cleanly and should be
    # terminated with extreme prejudice
    if i != 0:
      force_delete(branch)

    else:
      delete(branch, options.onto)

  return 0

if __name__ == "__main__":
  sys.exit(main(sys.argv))