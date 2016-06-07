#!/usr/bin/env python

'''
Script for wrapping commands that you want to pass a set of default arguments to.
'''

import collections, json, os, six, sys

def find_shadow():
    '''
    Find the first command in our PATH that we are shadowing.
    '''
    me = os.path.abspath(__file__)
    cmd = os.path.basename(me)
    path = os.environ.get('PATH', '').strip()
    if path == '':
        return None
    for root in path.split(':'):
        candidate = os.path.abspath(os.path.join(root, cmd))
        if os.path.exists(candidate) and os.access(candidate, os.R_OK|os.X_OK) and \
                not os.path.samefile(me, candidate):
            return candidate
    return None

def main(argv):

    cmd = os.path.basename(__file__)

    # Find the real version of whatever command we're wrapping.
    shadow = find_shadow()
    if shadow is None:
        sys.stderr.write('could not find %s\n' % cmd)
        return -1

    try:
        with open(os.path.expanduser('~/.notme.json')) as f:
            config = json.load(f)
        if not isinstance(config, collections.Mapping):
            raise TypeError('configuration is not a JSON object')
        for v in config.values():
            if not isinstance(v, collections.Iterable):
                raise TypeError('a configuration value is not a JSON list')
            if not all(isinstance(x, six.string_types) for x in v):
                raise TypeError('a configuration value item is not a JSON string')
    except Exception as e:
        sys.stderr.write('failed to parse configuration: %s\n' % e)
        return -1

    args = config.get(cmd, [])

    os.execv(shadow, [shadow] + args + argv[1:])

if __name__ == '__main__':
    sys.exit(main(sys.argv))
