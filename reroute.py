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

import argparse, functools, imp, os, pynotify, re, subprocess, sys

try:
    import psutil
except ImportError:
    psutil = None

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
    if psutil is None:
        raise Exception('psutil not available')
    procs = []
    for p in psutil.process_iter():
        if p.name == proc:
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

def find_shadow(target):
    '''
    Find the first command in our PATH that is not us.
    '''
    me = os.path.abspath(__file__)
    cmd = os.path.basename(me)
    path = os.environ.get('PATH', '').strip()
    if path == '':
        return None
    for root in path.split(':'):
        candidate = os.path.abspath(os.path.join(root, target))
        if os.path.exists(candidate) and os.access(candidate, os.R_OK|os.X_OK) and \
                not os.path.samefile(me, candidate):
            return candidate
    return None

api = {
    'error':functools.partial(_error, None),
    'find_shadow':find_shadow,
    'notify':functools.partial(_notify, None),
    'ps':ps,
    'run':run,
    'which':which,
}

def main(argv):
    error = functools.partial(_error, sys.stdout.isatty())
    api['error'] = error

    notify = functools.partial(_notify, sys.stdout.isatty())
    api['notify'] = notify

    # Figure out what command the user is trying to run.
    shortcut = os.path.basename(__file__)

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

    if shortcut.startswith('__'):
        # Avoid letting users accidentally invoke Python built ins.
        error('illegal shortcut name')
        return -1

    cmd = shortcuts.__dict__.get(shortcut)
    if cmd is None:
        error('shortcut %s not found' % shortcut)
        return -1

    try:
        if isinstance(cmd, str):
            target = find_shadow(cmd)
            if target is None:
                error('\'%s\' not found' % cmd)
                return -1
            os.execv(target, [target] + argv[1:])
        elif isinstance(cmd, list) and len(cmd) > 0:
            target = find_shadow(cmd[0])
            if target is None:
                error('\'%s\' not found' % cmd[0])
                return -1
            os.execv(target, [target] + cmd[1:] + argv[1:])
        elif hasattr(cmd, '__call__'):
            return cmd(api, argv)
        else:
            error('unrecognised type of shortcut %s' % shortcut)
            return -1
    except Exception as e:
        error('failed to run %s: %s' % (shortcut, e))
        return -1

if __name__ == '__main__':
    sys.exit(main(sys.argv))
