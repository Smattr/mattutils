#!/bin/bash

# Creates a setuid binary that executes a given shell command. This gives any
# arbitrary user the power to run the given command as you. Use it wisely.
# Matthew Fernandez, 2011

if [ -z "$1" ]; then
    echo "Usage: $0 command" >&2
    exit 1
fi

gcc -x c -o ${1// /} - <<EOF
#include <stdlib.h>

int main(int argc, char **argv) {
    return system("$1");
}
EOF
chmod ug+s ${1// /}

