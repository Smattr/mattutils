#!/usr/bin/env bash

# The sound of Australia

TMP=$(mktemp -d)
trap "rm -r ${TMP}" EXIT

# Scrape a couple of Triple J's feeds and FBi's beforecast.
python3 >${TMP}/playlist.pls <<EOT
import feedparser

def links(url):
    for entry in feedparser.parse(url).entries:
        if hasattr(entry, 'link'):
            yield entry.link
        if hasattr(entry, 'links'):
            for l in entry.links:
                yield l['href']
for url in ('http://www.cpod.org.au/feed.php?id=31',
            'http://www.abc.net.au/triplej/unearthed/podcast/mixtape/podcast.xml',
            'http://www.abc.net.au/triplej/unearthed/podcast/tops/podcast.xml'):
    for l in (l for l in links(url) if l.lower().endswith('.mp3')):
        print(l)
EOT

# Shuffle it all.
mplayer --cache=64 --shuffle --playlist=${TMP}/playlist.pls
