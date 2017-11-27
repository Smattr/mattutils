#!/usr/bin/env bash

if [ "$2" = "" ]; then
    echo "Usage: $0 inputfile outputfile" >&2
    exit 1
fi

which gs &>/dev/null || {
    echo "Ghostscript not found. Is it installed?" >&2;
    exit 1
}

gs -sDEVICE=pdfwrite -dCompatibilityLevel=1.4 -dPDFSETTINGS=/screen -dNOPAUSE -dQUIET -dBATCH -sOutputFile="$2" "$1"

