#!/bin/bash

# I use RSS feeds to monitor most of the sites I read regularly, but some sites
# don't provide a feed or host pages that I'm interested in, but are not
# covered by the feed. I use this script in a cron job to poll these pages
# regularly and notify me of updates.

if [ "$1" == "" ]; then
    echo "Usage: $0 URL" 1>&2
    exit 1
fi

# Directory to store past copies of files in.
CACHE="/var/tmp/has-changed-cache-${USER}"

mkdir -p "${CACHE}"

# You may want to change the hash command depending on your system.
CACHED_COPY="`echo $1 | sha1sum | cut -d\" \" -f 1`"

# Why doesn't wget have a PROPER option to rename the output?!?
WGET_TMP=`mktemp -d`
pushd ${WGET_TMP} >/dev/null
wget -N "$1" >/dev/null 2>/dev/null
popd >/dev/null

STATUS=0

if [ -e "${CACHE}/${CACHED_COPY}" ]; then
    diff "${CACHE}/${CACHED_COPY}" ${WGET_TMP}/* >/dev/null
    if [ $? != 0 ]; then
        echo "Modified page: $1."
        STATUS=2
    fi
else
    echo "New page (not in cache): $1."
    STATUS=1
fi

mv -f ${WGET_TMP}/* "${CACHE}/${CACHED_COPY}"
rmdir ${WGET_TMP}
exit ${STATUS}
