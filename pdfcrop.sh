#!/usr/bin/env bash

# Some PDFs come with obscenely large margins (I'm looking at you, research
# papers). Printing these 2-up and then trying to make out the minute font makes
# me cry, so this takes maximum advantage of the space available.

PDFVIEWER=evince

if [ -z "$1" ]; then
    echo "Usage: $0 pdffile" 1>&2
    exit 1
fi

for i in pdfcrop ${PDFVIEWER}; do
    which $i &>/dev/null || {
        echo "$i not found" >&2;
        exit 1;
    }
done

PDF_OUTPUT=`mktemp`
pdfcrop "$1" "${PDF_OUTPUT}" && \
 ${PDFVIEWER} "${PDF_OUTPUT}" && \
 rm -rf "${PDF_OUTPUT}"
