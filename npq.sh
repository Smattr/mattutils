#!/bin/sh

if [ $# -ne 1 ]; then
    echo "Usage: $0 printer_name" >&2
    exit 1
fi

which notify-send >/dev/null 2>/dev/null
if [ $? -ne 0 ]; then
    echo "notify-send not found. libnotify not installed?" >&2
    exit 2
fi

notify-send "$1 queue" "`lpq -P $1`"

