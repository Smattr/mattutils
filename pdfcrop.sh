#!/bin/sh

# Some PDFs come with obscenely large margins (I'm looking at you, research
# papers). Printing these 2-up and then trying to make out the minute font makes
# me cry, so this takes maximum advantage of the space available.

PDFVIEWER=evince

if [ -z "$1" ]; then
    echo "Usage: $0 pdffile" 1>&2
    exit 1
fi

# Check we have pdfcrop.
which pdfcrop >/dev/null 2>/dev/null
if [ $? != 0 ]; then
    echo "pdfcrop not found" 1>&2
    exit 1
fi

# Check your PDF viewer exists.
which ${PDFVIEWER} >/dev/null 2>/dev/null
if [ $? != 0 ]; then
    echo "${PDFVIEWER} not found" 1>&2
    exit 1
fi

PDF_OUTPUT=`mktemp`
pdfcrop "$1" "${PDF_OUTPUT}" && \
 ${PDFVIEWER} "${PDF_OUTPUT}" && \
 rm -rf "${PDF_OUTPUT}"
