#!/bin/bash

if [ "$1" == "" ]; then
    echo "Usage: $0 word [--force-command-line]" 1>&2
    exit 1
fi

which dict &>/dev/null
if [ $? -ne 0 ]; then
    echo "dict not found" 1>&2
    exit 1
fi

# WordNet (wn) seems to give nicer concise results than some of the others.
DEF=`dict -d wn $1 2>/dev/null`
if [ $? -ne 0 ]; then
    DEF="$1
No definition found."
else
    DEF=`echo "${DEF}" | tail --lines=+5`
fi

which notify-send &>/dev/null
if [ $? -eq 0 -a "$2" != "--force-command-line" ]; then
    notify-send "`echo "${DEF}" | head --lines=1`" "`echo "${DEF}" | tail --lines=+2`"
else
    # If we don't have notify-send, just spit out the results in the terminal.
    echo "${DEF}"
fi
