#!/usr/bin/env python

'''
Script for performing cursory validation of files.
'''

import magic, os, string, subprocess, sys

def run(cmd, **kwargs):
    with open(os.devnull, 'w') as null:
        try:
            subprocess.check_call(cmd, stdout=null, stderr=null, **kwargs)
            print '\033[32mok',
        except subprocess.CalledProcessError:
            print '\033[31mFAILED',
        finally:
            print '(%s)\033[0m' % ' '.join(cmd)

def main():
    if len(sys.argv) == 1:
        queue = ['.']
    else:
        queue = sys.argv[1:]

    ms = magic.open(magic.MAGIC_NONE)
    ms.load()

    while len(queue) > 0:
        item = queue.pop()
        print ' %s ...' % item,
        if not os.path.exists(item):
            print 'FAILED (does not exist)'
        elif os.path.isdir(item):
            dot_git = os.path.join(item, '.git')
            dot_hg = os.path.join(item, '.hg')
            if os.path.exists(dot_git) and os.path.isdir(dot_git):
                run(['git', 'fsck'], cwd=item)
            elif os.path.exists(dot_hg) and os.path.isdir(dot_hg):
                run(['hg', 'verify'], cwd=item)
            else:
                queue += map(lambda x: os.path.join(item, x), os.listdir(item))
                print 'recursing'
        else:
            filetype = ms.file(item)
            if filetype.startswith('Python script,'):
                run(['pylint', '--errors-only', item])
            elif filetype == 'OpenDocument Spreadsheet':
                run(['zip', '--test', item])
            elif filetype == 'XML document text' or filetype == 'SVG Scalable Vector Graphics image':
                run(['xmllint', '--noout', item])
            elif item.lower().endswith('.md'):
                # For some reason markdown is incorrectly identified as a Fortran program.
                run(['markdown', item])
            elif 'ASCII text' in filetype:
                with open(item, 'r') as f:
                    while True:
                        c = f.read(1)
                        if len(c) == 0:
                            print '\033[32mok (printable ASCII)\033[0m'
                            break
                        if not c in string.printable:
                            print '\033[31mFAILED (printable ASCII)\033[0m'
                            break
            else:
                print '\033[31mno checker found for \'%s\'\033[0m' % filetype



    return 0

if __name__ == '__main__':
    sys.exit(main())
