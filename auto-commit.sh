#!/bin/bash

# Matthew Fernandez <matthew.fernandez@gmail.com>
# October, 2011
#
# This script is designed to automatically sync a git repository across
# multiple machines. It has some gotchas, so I take no responsibility if you
# use it without understanding it and destroy your repo.

if [ $# -lt 1 ]; then
    echo "Usage: $0 repo_path [ check_server ]" >&2
    exit 1
fi

TMP_DIR=`mktemp -d`
LOG=${TMP_DIR}/log.txt
echo "$0 log" >${LOG}
trap "rm -rf ${TMP_DIR} ; exit" SIGINT SIGTERM EXIT

# Set a server to ping to check we are online before pushing and pulling to the
# given repo. You usually want this to be the hostname of the remote. This can
# be retrieved from git, but then we would also need to parse the user's SSH
# config to figure out where the hostname actually pointed.
if [ $# -gt 1 ]; then
    PING_SERVER=$2
else
    PING_SERVER=www.google.com
fi

# Commit message:
if [ -z "${USER}" ]; then
    USER=${LOGNAME}
fi
COMMIT_MESSAGE="Auto-commit from ${USER}@`uname --nodename`."

# Move into the repo.
cd "$1" &>/dev/null || {
    echo "Failed to change to $1." >&2 ;
    exit 1
}

# Add all outstanding changes.
echo " * git add *" >>${LOG}
git add --all . &>>${LOG}
if [ $? -ne 0 ]; then
    echo "Adding new files failed" >&2
    cat ${LOG} >&2
    exit 1
fi

# Commit outstanding changes.
echo " * git commit *" >>${LOG}
git commit -m "${COMMIT_MESSAGE}" &>>${LOG}
# We should probably attempt to detect commit failure here, but git returns the
# same error code for failure as for "nothing to commit."

# Check if we have a network.
ping -c 1 -w 5 ${PING_SERVER} &>/dev/null
if [ $? -ne 0 ]; then
    # No network.
    exit 0
fi

# Pull in new changes.
echo " * git pull *" >>${LOG}
git pull --rebase --ff-only --tags bitbucket master &>>${LOG}
if [ $? -ne 0 ]; then
    # If the pull failed, we may have been asked to resolve a conflict.
    echo " * git rebase abort *" >>${LOG}
    git rebase --abort &>>${LOG}
    echo "Rebase pull failed. Merge probably required." >&2
    cat ${LOG} >&2
    exit 4
fi

# Push any changes we just committed.
echo " * git push *" >>${LOG}
git push --tags bitbucket master &>>${LOG}
if [ $? -ne 0 ]; then
    echo "Pushing to remote failed. Merge probably required." >&2
    cat ${LOG} >&2
    exit 5
fi
