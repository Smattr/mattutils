#!/usr/bin/env python

'''
WIP

This script takes a playlist of music files and constructs a directory suitable
for serving a podcast from.
'''

import argparse, cgi, datetime, getpass, hashlib, os, random, subprocess, sys
from email.utils import formatdate
from mutagen.mp3 import MP3
import xml.etree.cElementTree as ET

def encode(text):
    return cgi.escape(text.decode('utf8')).encode('ascii', 'xmlcharrefreplace')

class Channel(object):
    def __init__(self, title, link, description, **kwargs):
        self.title = title
        self.link = link
        self.description = description
        self.kwargs = kwargs

    def attach(self, parent):
        c = ET.SubElement(parent, 'channel')
        ET.SubElement(c, 'title').text = encode(self.title)
        ET.SubElement(c, 'link').text = encode(self.link)
        for member, field in (('language', 'language'),
                              ('copyright', 'copyright'),
                              ('subtitle', 'itunes:subtitle'),
                              ('author', 'itunes:author'),
                              ('summary', 'itunes:summary')):
            if member in self.kwargs:
                ET.SubElement(c, field).text = encode(self.kwargs[member])
        return c

class Item(object):
    def __init__(self, title, url, length, type, date, **kwargs):
        self.title = title
        self.url = url
        self.length = length
        self.type = type
        self.date = date
        self.kwargs = kwargs

    def attach(self, parent):
        i = ET.SubElement(parent, 'item')
        ET.SubElement(i, 'title').text = self.title.decode('utf8')
        ET.SubElement(i, 'enclosure', url=self.url, length=self.length,
            type=self.type)
        ET.SubElement(i, 'pubDate').text = self.date
        for member, field in (('subtitle', 'itunes:subtitle'),
                              ('author', 'itunes:author'),
                              ('summary', 'itunes:summary'),
                              ('guid', 'guid'),
                              ('duration', 'itunes:duration')):
            if member in self.kwargs:
                ET.SubElement(i, field).text = encode(self.kwargs[member])
        return i

def digest(path):
    assert os.path.isfile(path)
    with open(path) as f:
        return hashlib.sha256(f.read()).hexdigest()

def main(argv):
    parser = argparse.ArgumentParser(
        description='turn a music playlist into a servable podcast')
    parser.add_argument('--input', '-i', type=argparse.FileType('r'),
        required=True, help='playlist to read')
    parser.add_argument('--output', '-o', required=True,
        help='directory to output to')
    parser.add_argument('--shuffle', action='store_true',
        help='randomise playlist')
    parser.add_argument('--prefix', required=True, help='webserver root')
    options = parser.parse_args(argv[1:])

    if not os.path.isdir(options.output):
        sys.stderr.write('%s is not a directory\n' % options.output)
        return -1

    for tool in ('ffmpeg', 'xmllint'):
        try:
            subprocess.check_call(['which', tool], stdout=subprocess.PIPE,
                stderr=subprocess.PIPE)
        except subprocess.CalledProcessError:
            sys.stderr.write('%s not found\n' % tool)
            return -1

    items = []

    start_time = datetime.datetime.now() + datetime.timedelta(hours=-1)

    lines = [x.strip() for x in options.input]
    if options.shuffle:
        random.shuffle(lines)

    for index, line in enumerate(lines):
        guid = digest(line)

        path = os.path.join(options.output, '%s.mp3' % guid)

        sys.stdout.write('Encoding %s...\n' % line)
        p = subprocess.Popen(['ffmpeg', '-y', '-i', line, '-ab', '192k', path],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        p.communicate()
        if p.returncode != 0:
            sys.stderr.write('Failed\n')
            return -1

        size = os.stat(path).st_size

        duration = int(MP3(path).info.length)

        timestamp = formatdate(float((start_time +
            datetime.timedelta(minutes=index)).strftime('%s')),
            datetime.tzinfo())

        items.append(Item(os.path.splitext(os.path.basename(line))[0],
            '%s%s.mp3' % (options.prefix, guid), str(size), 'audio/mpeg', timestamp,
            duration='%02d:%02d' % (duration // 60, duration % 60)))

    channel = Channel('My Music', options.prefix, 'My Music',
        author=getpass.getuser())
    rss = ET.Element('rss')
    c = channel.attach(rss)
    for i in items:
        i.attach(c)
    et = ET.ElementTree(rss)
    et.write('feed.xml', encoding='UTF-8', xml_declaration=True)

    # Write a Lighttpd conf.
    with open(os.path.join(options.output, 'lighttpd.conf'), 'w') as f:
        f.write('server.document-root = "%s"\n'
                'server.port = 8000\n'
                'mimetype.assign = (".mp3" => "audio/mpeg")\n' %
                os.path.abspath(options.output))

    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv))
