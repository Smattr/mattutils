#!/bin/bash

# Grab a list of a user's bitbucket repos.

if [ $# -ne 3 ]; then
    echo "Usage: $0 user username password" >&2
    exit 1
fi

wget --output-document=- --http-user="$2" --http-password="$3" "https://api.bitbucket.org/1.0/user?username=$1" 2>/dev/null \
 | grep --color=never "\"name\":" \
 | sed 's/.*"name": "\(.*\)".*/\1/g'
