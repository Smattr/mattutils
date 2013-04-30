#!/bin/bash

# Infinitely cat some sources into a FIFO. Useful for faking `include` in conf
# files that have no support for this.

if [ $# -lt 2 ]; then
    echo "Usage: $0 target inputs..." >&2
    exit 1
fi

if [ ! -p "$1" ]; then
    echo "$1 does not exist or is not a FIFO" >&2
    exit 1
fi

TARGET=$1
shift
trap 'exit 0' INT TERM QUIT
while true; do
    cat "$@" >"${TARGET}"
done
