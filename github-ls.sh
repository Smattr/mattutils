#!/bin/bash

# A quick way to grab the names of all your github repos.

if [ ! -n "$1" ]; then
    echo "Usage: $0 username" >&2
    exit 1
fi

wget --output-document=- "https://github.com/api/v2/xml/repos/show/$1" 2>/dev/null \
| grep --color=never "^    <url>.*</url>$" \
| sed 's/    <url>https:\/\/github\.com\/'"${1}"'\/\(.*\)<\/url>/\1/'

