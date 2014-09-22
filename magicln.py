#!/usr/bin/env python

'''
Create symlinks to this script from targets that you want to map to different
executables depending on your current working directory. See the logic below
for how to format ~/.magicln.conf.
'''

import ConfigParser, os, subprocess, sys

CONFIG_FILE = os.path.expanduser('~/.magicln.conf')

def main():
    parser = ConfigParser.SafeConfigParser()
    try:
        parser.read(CONFIG_FILE)
    except Exception as e:
        print >>sys.stderr, 'failed to read config: %s' % str(e)
        return -1

    symbol = os.path.basename(sys.argv[0])

    d = os.getcwd()
    target = None
    while d != '/':
        try:
            target = parser.get(symbol, d)
            break
        except ConfigParser.NoSectionError:
            print >>sys.stderr, 'no targets for %s' % symbol
            return -1
        except ConfigParser.NoOptionError:
            d = os.path.dirname(d)

    if target is None:
        print >>sys.stderr, 'no target found for %s' % symbol
        return -1

    if not os.path.exists(target):
        print >>sys.stderr, 'target of \'%s\' found, but this doesn\'t exist' % target
        return -1

    return subprocess.call([target] + sys.argv[1:])

if __name__ == '__main__':
    sys.exit(main())
