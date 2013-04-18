#!/usr/bin/env python

# ABC streams their news 24 channel at http://www.abc.net.au/news/abcnews24/.
# Unfortunately the web interface doesn't work particularly well on low powered
# machines. This script utilises the iPhone stream to let you watch it on
# mplayer.

import os, shutil, subprocess, sys, threading, tempfile, urllib2

PLAYLIST = 'http://iphonestreaming.abc.net.au/news24/news24_hi.m3u8'

MPLAYER = ['mplayer', '-cache-min', '0', '-mc', '0.1', '-fs', '%(file)s']

DEBUG = False

def get_stream(url):
    req = urllib2.Request(url)
    return urllib2.urlopen(req)

def get_playlist():
    # When debugging, `wget PLAYLIST -O test.m3u3`
    if DEBUG:
        with open('test.m3u3', 'r') as f:
            return f.read()

    return get_stream(PLAYLIST).read()

def writer(pipe):
    seen = set()
    w = open(pipe, 'w')
    while True:
        if DEBUG: print >>sys.stderr, 'Getting new playlist'

        for line in map(lambda x:x.strip(), get_playlist().split('\n')):
            if not line or line.startswith('#'): # Skip comments.
                continue

            # Track which segments we've already copied in case the playlist
            # returns duplicates.
            if line in seen:
                continue
            else:
                seen.add(line)

            if DEBUG: print >>sys.stderr, 'Getting %s' % line

            # Copy this segment to mplayer.
            try:
                f = get_stream(line)
                shutil.copyfileobj(f, w)
            except Exception as inst:
                print >>sys.stderr, 'Failed to retrieve %s: %s' % (line, inst)

def subst(xs, values):
    return map(lambda x: x % values, xs)

def main():
    seen = set()

    d = tempfile.mkdtemp()
    pipe = os.path.join(d, 'pipe')
    os.mkfifo(pipe)

    mplayer = subprocess.Popen(subst(MPLAYER, {'file':pipe}))

    # Need to do the pipe operations in a separate thread because they will
    # block.
    t = threading.Thread(target=writer, args=[pipe])
    t.daemon = True
    t.start()

    mplayer.wait()
    shutil.rmtree(d)
    return 0

if __name__ == '__main__':
    sys.exit(main())
