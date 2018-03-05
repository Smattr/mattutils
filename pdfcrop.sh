#!/usr/bin/env bash

# Some PDFs come with obscenely large margins (I'm looking at you, research
# papers). Printing these 2-up and then trying to make out the minute font makes
# me cry, so this takes maximum advantage of the space available.

PDFVIEWER=evince

if [ -z "$1" ]; then
  printf "Usage: %s pdffile\n" "$0" >&2
  exit 1
fi

for i in pdfcrop ${PDFVIEWER}; do
  if ! which $i &>/dev/null; then
    printf "%s not found\n" "$i" >&2;
    exit 1;
  fi
done

PDF_OUTPUT=$(mktemp)
pdfcrop "$1" "${PDF_OUTPUT}" && \
 ${PDFVIEWER} "${PDF_OUTPUT}" && \
 rm -rf "${PDF_OUTPUT}"
