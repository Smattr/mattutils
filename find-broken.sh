#!/usr/bin/env bash

# Finds broken symlinks.

if [ $# -gt 0 ]; then
    DIR=$1
else
    DIR=.
fi

find "${DIR}" -type l -exec sh -c '[ -e "{}" ] || echo "{} is broken"' \;

