#!/usr/bin/env bash

# Sometimes you want to know the size of of a C entity that cannot be
# determined using `sizeof` alone. In my original case, I wanted to know the
# size of a compiled function. This script allows you to retrieve this
# information and dump it to a C file.
#
# E.g. suppose you need to know the size of a function, foo. You write code
# that looks like this:
#
#  extern size_t sizeof_foo;
#  ...
#  printf("foo is %lu bytes\n", sizeof_foo);
#
# Then you compile the source containing foo and run this script on it that
# generates a list of size_t variables. Note that the source containing foo can
# even be the one you're printing from. You simply need to link its compiled
# object file with the output of this script.
#
# Obviously there are caveats to this technique (inlined functions, specialised
# functions, assembly functions, ...), but I assume if you are using this you
# are sensible enough to figure these out for yourself.


if [ $# -ne 1 ]; then
    echo "Usage: $0 object_file" >&2
    echo " Retrieves symbol size information from an object file." >&2
    exit 1
fi

${TOOLPREFIX}objdump --sym "$1" | \
    grep --color=none --basic-regexp '^[0-9]\+' | \
    sed -e 's/^[0-9]\+[^0-9]*\([0-9]\+\) \(.*\)$/size_t sizeof_\2 = \1;/g' -e 's/\./_/g'
