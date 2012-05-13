#!/bin/bash

# Matthew Fernandez <matthew.fernandez@gmail.com>
# October, 2011
#
# This script is designed to automatically sync a git repository across
# multiple machines. It has some gotchas, so I take no responsibility if you
# use it without understanding it and destroy your repo.

if [ $# -ne 1 ]; then
    echo "Usage: $0 repo_path" >&2
    exit 1
fi

# Commit message:
COMMIT_MESSAGE="Auto-commit from ${USER}@`uname --nodename`."

# Move into the repo.
cd "$1" &>/dev/null || {
    echo "Failed to change to $1." >&2 ;
    exit 1
}

# Add all outstanding changes.
git add --all . >/dev/null
if [ $? -ne 0 ]; then
    echo "Adding new files failed" >&2
    exit 1
fi

# Commit outstanding changes.
git commit -m "${COMMIT_MESSAGE}" >/dev/null
# We should probably attempt to detect commit failure here, but git returns the
# same error code for failure as for "nothing to commit."

# Check if we have a network.
ping -c 1 -w 1 www.google.com &>/dev/null
if [ $? -ne 0 ]; then
    # No network.
    exit 0
fi

# Pull in new changes.
git pull --rebase --ff-only origin master &>/dev/null
if [ $? -ne 0 ]; then
    # If the pull failed, we may have been asked to resolve a conflict.
    git rebase --abort &>/dev/null
    echo "Rebase pull failed. Merge probably required." >&2
    exit 4
fi

# Push any changes we just committed.
git push origin master &>/dev/null
if [ $? -ne 0 ]; then
    echo "Pushing to remote failed. Merge probably required." >&2
    exit 5
fi

