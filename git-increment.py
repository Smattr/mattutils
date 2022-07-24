#!/usr/bin/env python3

"""
Create a new branch off an existing one, for addressing feedback

When iterating through rounds of feedback with code reviewers, it is often
useful to keep track of the previous state(s) of a branch. This script assumes
you want to do this by suffixing “-2”, “-3”, “-4”, … to your branches as you go
through successive rounds of review. This lines up with git-cleanup.py, that
gives you a way to purge such named branches.
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
  parser.add_argument("branch", help="branch to increment")
  options = parser.parse_args(args[1:])

  # check this is actually a Git repository
  run(["git", "rev-parse", "HEAD"])

  # is the working directory clean?
  changes = call(["git", "status", "--short", "--ignore-submodules"])
  if re.search(r"^.[^\?]", changes, flags=re.MULTILINE) is not None:
    sys.stderr.write("changes in working directory; aborting\n")
    return -1
  del changes

  # check this branch exists upstream
  upstream = call(["git", "ls-remote", "origin", options.branch])
  if len(upstream.strip()) == 0:
    sys.stderr.write(f"\033[31;1mWARNING:\033[0m {options.branch} does not "
                     "exist upstream")
    commit = None
  else:
    commit = upstream.split()[0]
  del upstream

  # find the latest local branch for this upstream branch
  local_branch = options.branch
  try:
    local_commit = call(["git", "rev-parse", local_branch])
  except sp.CalledProcessError:
    sys.stderr.write(f"branch {local_branch} does not exist")
    return 1
  counter = 2
  while True:
    candidate = f"{options.branch}-{counter}"
    try:
      local_commit = call(["git", "rev-parse", candidate])
    except sp.CalledProcessError:
      break
    local_branch = candidate
    counter += 1

  # check it matches upstream
  if commit is not None and commit != local_commit:
    sys.stderr.write(f"branch {local_branch} is not at the same commit as "
                     f"upstream {options.branch}")
    return 1

  # move to it and do the increment
  new = f"{options.branch}-{counter + 1}"
  run(["git", "checkout", "-b", new, local_branch])

  return 0

if __name__ == "__main__":
  sys.exit(main(sys.argv))
