#!/usr/bin/env python

"""
dd is a very handy tool for copying raw data from one device/file to another,
but it eternally irks me that it doesn't have an option to display progress
while copying. Instead it has a strange mechanism where it will output its
current progress when it receives SIGUSR1. This script wraps dd and sends it
SIGUSR1 once a second to generate progress updates.
"""
import signal
import sys
import subprocess
import threading
import time

p = None
lock = None
reset_cursor = False

def emit_progress():
    global p, lock, reset_cursor
    while p and not p.poll():
        # Notify the main thread that the cursor position needs to be reset.
        lock.acquire()
        reset_cursor = True
        lock.release()

        p.send_signal(signal.SIGUSR1)
        time.sleep(1)

def signal_passthrough(sig, frame):
    # Handle a signal by passing it through to dd.
    global p
    if p and not p.poll():
        p.send_signal(sig)

def main():
    global p, lock, reset_cursor

    # Handball SIGTERM and SIGUSR1 to dd.
    signal.signal(signal.SIGTERM, signal_passthrough)
    signal.signal(signal.SIGINT, signal_passthrough)

    lock = threading.Lock()
    t = threading.Thread(target=emit_progress)

    p = subprocess.Popen(['dd'] + sys.argv[1:], stdout=subprocess.PIPE)

    # Create some lines in the terminal that we will overwrite with progress
    # updates.
    print '\n\n'

    # Wait a little bit before starting the signaller to make sure dd installs
    # its handler before we signal it. Otherwise we end up killing dd
    # immediately.
    time.sleep(1)
    t.start()

    while True:
        c = p.stdout.read(1)

        # If we've just signalled dd, the data we're receiving will be the next
        # progress update so we need to jump back and up three lines in order
        # to overwrite the last progress update. Assume that 100 characters
        # is longer than any progress line dd has printed.
        if reset_cursor:
            lock.acquire()
            sys.stdout.write('[100D[3A[J')
            sys.stdout.flush()
            reset_cursor = False
            lock.release()

        if c:
            sys.stdout.write(c)
        elif p.poll():
            break
    t.join()
    p.wait()
    return 0

if __name__ == '__main__':
    sys.exit(main())
