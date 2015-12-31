#!/usr/bin/env python

'''
Script for performing cursory validation of files.
'''

import argparse, json, magic, os, string, subprocess, sys

NOT_FOUND = -3
NO_CHECKER = -2
FAILED = -1
OK = 0

def run(cmd, **kwargs):
    with open(os.devnull, 'w') as null:
        try:
            subprocess.check_call(cmd, stdout=null, stderr=null, **kwargs)
            print '\033[32mok (%s)\033[0m' % ' '.join(cmd)
            return OK
        except subprocess.CalledProcessError:
            print '\033[31mFAILED (%s)\033[0m' % ' '.join(cmd)
            return FAILED

def main():
    parser = argparse.ArgumentParser(description='file validator')
    parser.add_argument('--db', type=argparse.FileType('r+'),
        help='database to update')
    parser.add_argument('input', nargs='*', default=[],
        help='path or directory to scan')
    opts = parser.parse_args(sys.argv[1:])

    if len(opts.input) == 0:
        queue = ['.']
    else:
        queue = opts.input

    ms = magic.open(magic.MAGIC_NONE)
    ms.load()

    if opts.db is None:
        results = {}
    else:
        results = json.load(opts.db)

    while len(queue) > 0:
        item = os.path.abspath(queue.pop())
        print ' %s ...' % item,
        if not os.path.exists(item):
            results[item] = NOT_FOUND
            print '\033[31mFAILED (does not exist)\033[0m'
        elif os.path.isdir(item):
            dot_git = os.path.join(item, '.git')
            dot_hg = os.path.join(item, '.hg')
            if os.path.exists(dot_git) and os.path.isdir(dot_git):
                results[item] = run(['git', 'fsck'], cwd=item)
            elif os.path.exists(dot_hg) and os.path.isdir(dot_hg):
                results[item] = run(['hg', 'verify'], cwd=item)
            else:
                queue += map(lambda x: os.path.join(item, x), os.listdir(item))
                print 'recursing'
        else:
            filetype = ms.file(item)
            if filetype.startswith('Python script,'):
                results[item] = run(['pylint', '--errors-only', item])
            elif filetype == 'OpenDocument Spreadsheet':
                results[item] = run(['zip', '--test', item])
            elif filetype == 'XML document text' or filetype == 'SVG Scalable Vector Graphics image':
                results[item] = run(['xmllint', '--noout', item])
            elif item.lower().endswith('.md'):
                # For some reason markdown is incorrectly identified as a Fortran program.
                results[item] = run(['markdown', item])
            elif 'ASCII text' in filetype:
                with open(item, 'r') as f:
                    while True:
                        c = f.read(1)
                        if len(c) == 0:
                            print '\033[32mok (printable ASCII)\033[0m'
                            results[item] = OK
                            break
                        if not c in string.printable:
                            print '\033[31mFAILED (printable ASCII)\033[0m'
                            results[item] = FAILED
                            break
            elif os.path.splitext(item.lower())[1] in ('.avi',):
                # Many media container formats are misidentified by magic.
                results[item] = run(['ffmpeg', '-i', item, '-f', 'null', os.devnull])
            else:
                print '\033[31mno checker found for \'%s\'\033[0m' % filetype
                results[item] = NO_CHECKER

    if opts.db is None:
        print json.dumps(results, indent=2)
    else:
        json.dump(results, opts.db, indent=2)

    return 0

if __name__ == '__main__':
    sys.exit(main())
