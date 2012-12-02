#!/bin/bash

# Updates a working directory to the latest tip/head of default/master.
# Assumes your working directory is clean.

if [ $# -ne 1 ]; then
    echo "Usage: $0 working_dir" >&2
    exit 1
fi

pushd "$1" &>/dev/null
if [ $? -ne  0 ]; then
    echo "Failed to change to $1" >&2
    exit 1
fi

if [ -e ".git" ]; then
    # This is a git repository.
    CUR="`git log -1 --oneline`"
    git pull origin master &>/dev/null
    if [ $? -ne 0 ]; then
        echo "Failed to pull and update" >&2
        exit 1
    fi
    NOW="`git log -1 --oneline`"
elif [ -e ".hg" ]; then
    # This is a mercurial repository.
    CUR=`hg identify -i`
    CUR="${CUR} `hg log -r ${CUR} --template '{desc|firstline}'`"
    hg pull -u &>/dev/null
    if [ $? -ne 0 ]; then
        echo "Failed to pull and update" >&2
        exit 1
    fi
    NOW=`hg identify -i`
    NOW="${NOW} `hg log -r ${NOW} --template '{desc|firstline}'`"
else
    echo "Unidentified repository type" >&2
    exit 1
fi

if [ "${CUR}" != "${NOW}" ]; then
    echo "${CUR} -> ${NOW}"
fi

popd &>/dev/null
