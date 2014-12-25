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

import argparse, imp, os, psutil, pynotify, subprocess, sys

_pynotify_inited = False

def error(message):
    '''Output an error message, taking into account whether we are running on
    the command line or via a GUI.'''
    global _pynotify_inited
    if sys.stderr.isatty():
        print >>sys.stderr, message
    else:
        if not _pynotify_inited:
            pynotify.init('reroute')
            _pynotify_inited = True
        n = pynotify.Notification('reroute', message)
        n.set_urgency(pynotify.URGENCY_CRITICAL)
        n.show()

def notify(message):
    global _pynotify_inited
    if sys.stdout.isatty():
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
    p = subprocess.Popen(cmd, shell=isinstance(cmd, str),
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
        'error':error,
        'notify':notify,
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
    parser.add_argument('shortcut', nargs='?', help='shortcut to invoke')
    opts = parser.parse_args(argv[1:])

    try:
        shortcuts = imp.load_source('', os.path.expanduser('~/.reroute-config.py'))
    except (ImportError, IOError):
        shortcuts = None

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

    if isinstance(cmd, str):
        return subprocess.call(cmd, shell=True)
    elif isinstance(cmd, list):
        return subprocess.call(cmd)
    elif hasattr(cmd, '__call__'):
        return cmd(api)
    else:
        error('unrecognised type of shortcut %s' % opts.shortcut)
        return -1

if __name__ == '__main__':
    sys.exit(main(sys.argv))
