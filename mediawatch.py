#!/usr/bin/env python

"""
This script scans a given list of directories and reports on changes noted
since the last run.
"""

import os.path
import sys
import getopt
import hashlib

NEW_FILE = 0
MODIFIED_FILE = 1
REMOVED_FILE = 2
UNCHANGED_FILE = 3

READ_BUFFER = 10240

def parseArguments(options):
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'd:p:r:', ['database=', \
            'path=', 'replace='])
        if args:
            raise 'Extra arguments on command line'
        for o, a in opts:
            if o in ['-d', '--database']:
                options['database'] = a
            elif o in ['-p', '--path']:
                options['paths'].append(a)
            elif o in ['-r', '--replace']:
                if '=' not in a:
                    raise 'Malformed replace argument'
                options['replacements'][a.split('=')[0]] = '='.join(a.split('=')[1:])
            else:
                raise 'Unexpected argument'
    except:
        return False
    return True

def getHash(filepath):
    try:
        hash = hashlib.md5()
        f = open(filepath, 'rb')
        data = f.read(READ_BUFFER)
        while data:
            hash.update(data)
            data = f.read(READ_BUFFER)
        f.close()
        return hash.hexdigest()
    except:
        # If we can't read the file, that's not a critical error.
        # FIXME: command line opts for these msgs.
        sys.stderr.write('Warning: failed to hash %s: %s\n' % \
            (filepath, sys.exc_info()[0]))
        return ''

def updateFileTable(table, path):
    try:
        for file in os.listdir(path):
            if file.startswith('.'):
                # Ignore hidden files. TODO: Make this a command line opt.
                continue
            file = os.path.join(path, file)
            if os.path.isdir(file):
                updateFileTable(table, file)
            elif file in table:
                if str(os.path.getmtime(file)) == table[file]['modified']:
                    table[file]['state'] = UNCHANGED_FILE
                    if not table[file]['hash']:
                        # We may have failed to hash this file previously due
                        # to permissions (or I/O errors as happened to me) in
                        # which case we may as well try again.
                        table[file]['hash'] = getHash(file)
                else:
                    table[file]['state'] = MODIFIED_FILE
                    table[file]['modified'] = str(os.path.getmtime(file))
                    table[file]['hash'] = getHash(file)
            else:
                table[file] = {'path':file, \
                               'modified':str(os.path.getmtime(file)), \
                               'hash':getHash(file), \
                               'state':NEW_FILE}
    except:
        sys.stderr.write('Warning: failed reading from directory %s: %s\n' % \
            (path, sys.exc_info()[0]))

def apply_replacements(path, replacements):
    for prefix in replacements:
        if path.startswith(prefix):
            path = '%s%s' % (replacements[prefix], path[len(prefix):])
                break
    return path

def main():
    # Setup some default options.
    options = {
        'database':None,
        'paths':[],
        'replacements':{},
    }

    if not parseArguments(options) or \
       not options['database'] or \
       not options['paths']:
        sys.stderr.write(\
"""Usage: %s options
 Options:
  -d file | --database=file   File to read/write file information from/to, recording data from last time and this pass. This option is required.
  -p path | --path=path       A path to examine. This option must be used at least once, but can also be used multiple times.
  -r find=replace | --replace=find=replace Replace the prefix 'find' if at the start of a modified file with the prefix 'replace'. This argument can be used multiple times.
""" % sys.argv[0])
        return -1

    # A dictionary of files keyed on the path.
    files = {}

    if os.path.exists(options['database']):
        # If the database doesn't exist, assume this is the first time this has
        # been run.
        try:
            f = open(options['database'], 'r')
            for line in f:
                if not line.strip() or line.startswith('#'):
                    # Allow comments and blank lines.
                    continue
                d = dict(zip(['hash', 'path', 'modified'], \
                    line.strip().split('|')))
                d['state'] = REMOVED_FILE
                files[d['path']] = d
            f.close()
        except:
            sys.stderr.write('Error loading database %s: %s\n' % \
                (options['database'], sys.exc_info()[0]))
            return -1

    for p in options['paths']:
        updateFileTable(files, p)

    # Write output where necessary.
    try:
        f = open(options['database'], 'w')
        for key in sorted(files.keys()):
            if files[key]['state'] != REMOVED_FILE:
                f.write('%s|%s|%s\n' % (files[key]['hash'], key,
                    files[key]['modified']))
            if files[key]['state'] == NEW_FILE:
                sys.stdout.write('+ %s\n' % \
                    apply_replacements(key, options['replacements']))
            elif files[key]['state'] == MODIFIED_FILE:
                sys.stdout.write('M %s\n' % \
                    apply_replacements(key, options['replacements']))
            elif files[key]['state'] == REMOVED_FILE:
                sys.stdout.write('- %s\n' % \
                    apply_replacements(key, options['replacements']))
        f.close()
    except:
        sys.stderr.write('Failed to write to database %s: %s.\n' % \
            (options['database'], sys.exc_info()[0]))
        return -1

    return 0

if __name__ == '__main__':
    sys.exit(main())
