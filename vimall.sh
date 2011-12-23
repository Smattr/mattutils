#!/bin/bash

# Open all the files in a directory in vim in series.

if [ -n "$1" ]; then
    DIR="$1"
else
    DIR="."
fi

find "${DIR}" \
 -type f \
 ! -path '*/.hg/*' \
 ! -path '*/.git/*' \
 -exec vim '{}' \;
