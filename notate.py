#!/usr/bin/env python

'''
Script for taking notes on PDFs. I don't really have a good way of doing this
currently, but at least this script automates the process of splitting the
pages, scribbling on them and then putting them back together.
'''

import argparse, curses, os, re, shutil, subprocess, sys

def pdf_split(pdf, dest):
    '''Split a PDF into pages.'''
    subprocess.check_call(['pdftk', os.path.abspath(pdf), 'burst'],
        cwd=os.path.abspath(dest))

def pages(path):
    '''Return the pages in a given directory.'''
    pattern = re.compile('pg_[0-9][0-9][0-9][0-9].pdf')
    for p in os.listdir(path):
        if pattern.match(p):
            yield os.path.abspath(os.path.join(path, p))

def contains_pages(path):
    '''Returns true if a directory contains pages of a split PDF.'''
    return len(list(pages(path))) != 0

def pdf_reassemble(path, pdf):
    '''Reassemble a collection of pages into a PDF.'''
    ps = list(pages(path))
    ps.sort()
    subprocess.check_call(['pdftk'] + ps + ['cat', 'output',
        os.path.abspath(pdf)])

def pdf_edit(pdf):
    '''Edit a given pdf page.'''
    subprocess.check_call(['inkscape', os.path.abspath(pdf)])

def init():
    '''Initialise curses'''
    stdscr = curses.initscr()
    curses.start_color()
    curses.noecho()
    curses.cbreak()
    stdscr.keypad(1)
    curses.curs_set(0)
    return stdscr

def deinit(screen):
    curses.nocbreak()
    screen.keypad(0)
    curses.echo()
    curses.endwin()

def main(argv):
    parser = argparse.ArgumentParser(description='notation tool for PDFs')
    parser.add_argument('file', help='file to markup')
    opts = parser.parse_args(argv[1:])

    if not os.path.exists(opts.file):
        print >>sys.stderr, '%s not found' % opts.file
        return -1

    with open(os.devnull, 'w') as null:
        ret = subprocess.call(['which', 'pdftk'], stdout=null, stderr=null)
        if ret != 0:
            print >>sys.stderr, 'pdftk not found'
            return -1
        ret = subprocess.call(['which', 'inkscape'], stdout=null, stderr=null)
        if ret != 0:
            print >>sys.stderr, 'inkscape not found'
            return -1

    opts.file = os.path.abspath(opts.file)

    metadir = os.path.join(os.path.dirname(opts.file), '.notate',
        os.path.basename(opts.file))
    if not os.path.exists(metadir):
        os.makedirs(metadir)

    if not contains_pages(metadir):
        print 'splitting PDF...'
        pdf_split(opts.file, metadir)

    pgs = list(pages(metadir))
    pgs.sort()
    selected = 0

    screen = init()

    # TODO: pagination

    while True:
        screen.clear()

        screen.addstr(1, 2, 'n = take notes | q = quit | r = reassemble PDF', 0)
        for i, p in enumerate(pgs):
            style = curses.A_REVERSE if i == selected else 0
            screen.addstr(3 + i, 2, p, style)

        c = screen.getch()

        if c == curses.KEY_UP:
            if selected != 0:
                selected -= 1

        elif c == curses.KEY_DOWN:
            if selected != len(pgs) - 1:
                selected += 1

        elif c == ord('n'):
            deinit(screen)
            print 'Editing %s...' % pgs[selected]
            pdf_edit(pgs[selected])
            screen = init()

        elif c == ord('q'):
            break

        elif c == ord('r'):
            deinit(screen)
            print 'Reassembling...',
            pdf_reassemble(metadir, opts.file)
            raw_input('Done. Hit enter to continue.')
            screen = init()

    deinit(screen)

    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv))
