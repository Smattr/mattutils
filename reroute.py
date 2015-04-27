#!/usr/bin/env python

'''
Shortcut rerouter.

I use a variety of window managers across my machines, each of which has a
different way of configuring keyboard shortcuts. Some of them also fail to
notice updates to these configurations without a restart. To work around this I
first setup all my relevant shortcuts to point at this script with a command
line argument indicating the key combination. Then I write my actual shortcuts
in ~/.reroute-config.py. This way I can tweak shortcuts whenever I need, sync
my shortcuts between computers and window managers and generally suffer fewer
surprises.

A shortcut in ~/.reroute-config.py can be:
 1. A string to be executed via a shell;
 2. A list of arguments to execute directly; or
 3. A Python function to run.
'''

import argparse, functools, imp, os, psutil, pynotify, re, subprocess, sys

_pynotify_inited = False

def _error(tty, message):
    '''Output an error message, taking into account whether we are running on
    the command line or via a GUI.'''
    global _pynotify_inited
    if tty or (tty is None and sys.stderr.isatty()):
        print >>sys.stderr, message
    else:
        if not _pynotify_inited:
            pynotify.init('reroute')
            _pynotify_inited = True
        n = pynotify.Notification('reroute', message)
        n.set_urgency(pynotify.URGENCY_CRITICAL)
        n.show()

def _notify(tty, message):
    global _pynotify_inited
    if tty or (tty is None and sys.stdout.isatty()):
        print >>sys.stdout, message
    else:
        if not _pynotify_inited:
            pynotify.init('reroute')
            _pynotify_inited = True
        n = pynotify.Notification('reroute', message)
        n.show()

def ps(proc):
    procs = []
    for p in psutil.process_iter():
        if p.name() == proc:
            procs.append(p)
    return procs

def run(cmd):
    p = subprocess.Popen(cmd,
        shell=(isinstance(cmd, str) or isinstance(cmd, unicode)),
        stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = p.communicate()
    return p.returncode, stdout, stderr

def which(cmd):
    try:
        return subprocess.check_output(['which', cmd], stderr=subprocess.PIPE)
    except subprocess.CalledProcessError:
        return ''

api = {
    1:{
        'error':functools.partial(_error, None),
        'notify':functools.partial(_notify, None),
        'ps':ps,
        'run':subprocess.call,
        'which':which,
    },
    2:{
        'run':run,
    }
}

def main(argv):
    parser = argparse.ArgumentParser(description='shortcut gateway')
    parser.add_argument('--list', '-l', action='store_true',
        help='show available shortcuts')
    parser.add_argument('--tty', action='store_true', default=None,
        help='force output to be recognised as a TTY')
    parser.add_argument('--no-tty', dest='tty', action='store_false',
        help='force output to not be recognised as a TTY')
    parser.add_argument('shortcut', nargs='?', help='shortcut to invoke')
    opts = parser.parse_args(argv[1:])

    error = functools.partial(_error, opts.tty)
    api[1]['error'] = error

    notify = functools.partial(_notify, opts.tty)
    api[1]['notify'] = notify

    try:
        shortcuts = imp.load_source('', os.path.expanduser('~/.reroute-config.py'))
    except (ImportError, IOError):
        shortcuts = None
    except SyntaxError:
        error('malformed configuration file')
        return -1

    if shortcuts is None:
        error('no shortcuts defined')
        return -1

    if opts.list:
        available = []
        for k, v in shortcuts.__dict__.items():
            if k.startswith('__'):
                continue
            if isinstance(v, str):
                available.append('%s - exec "%s"' % (k, v))
            elif isinstance(v, list):
                available.append('%s - exec "%s"' % (k, ' '.join(v)))
            elif hasattr(v, '__call__'):
                available.append('%s - %s' % (k, v.__doc__))
            else:
                available.append('%s - unknown' % k)
        notify('\n'.join(available))
        return 0

    if opts.shortcut is None:
        error('no shortcut specified')
        return -1

    if opts.shortcut.startswith('__'):
        # Avoid letting users accidentally invoke Python built ins.
        error('illegal shortcut name')
        return -1

    cmd = shortcuts.__dict__.get(opts.shortcut)
    if cmd is None:
        error('shortcut %s not found' % opts.shortcut)
        return -1

    try:
        if isinstance(cmd, str):
            return subprocess.call(cmd, shell=True)
        elif isinstance(cmd, list):
            return subprocess.call(cmd)
        elif hasattr(cmd, '__call__'):
            return cmd(api)
        else:
            error('unrecognised type of shortcut %s' % opts.shortcut)
            return -1
    except Exception as e:
        error('failed to run %s: %s' % (opts.shortcut, e))
        return -1

if __name__ == '__main__':
    sys.exit(main(sys.argv))
