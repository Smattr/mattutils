#!/bin/bash

# A quick way to check whether the C bindings for a library are installed.

# C compiler to use.
CC=gcc

if [ ! -n "$1" ]; then
    echo "Usage: $0 library" 1>&2
    exit 1
fi

${CC} -l"$1" -x c -o /dev/null - &>/dev/null <<EOT
int main(int argc, char** argv) {
    return 0;
}
EOT
if [ $? -gt 0 ]; then
    echo "Library $1 not found" 1>&2
    exit 1
fi

