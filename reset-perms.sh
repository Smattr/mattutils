#!/bin/bash

if [ -z "$1" -o -z "$2" -o -z "$3" ]; then
    echo "Usage: $0 path directory_mask file_mask" 1>&2
    exit 1
fi

find "$1" \( -type d -and -exec chmod $2 "{}" \; \) -or \
          \( -type f -and -exec chmod $3 "{}" \; \)
