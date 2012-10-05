#!/bin/bash

# A quick way to grab the names of all your github repos.
# Python intercession is necessary because github returns JSON that looks like shit.

if [ ! -n "$1" ]; then
    echo "Usage: $0 username" >&2
    exit 1
fi

wget --output-document=- "https://api.github.com/users/$1/repos" 2>/dev/null \
 | python -mjson.tool \
 | grep --color=never "\"name\":" \
 | sed 's/.*"name": "\(.*\)".*/\1/g'

