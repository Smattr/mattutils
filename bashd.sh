#!/bin/bash

# This script connects a port on your computer to a bash instance. I use it for
# things like probing the environment in my crontab. Run it with a chosen port
# and then run "nc localhost port" in another terminal to connect.
#
#Don't run this if you don't understand what it's doing, as it effectively
# creates a backdoor into your machine.

if [ $# -ne 1 ]; then
    echo "Usage: $0 port" >&2
    exit 1
fi

TMP_DIR=`mktemp -d`
mkfifo "${TMP_DIR}/bashd"
bash <"${TMP_DIR}/bashd" 2>&1 | nc -l localhost $1 2>&1 >"${TMP_DIR}/bashd"
rm -rf "${TMP_DIR}"

