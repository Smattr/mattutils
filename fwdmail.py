#!/usr/bin/env python

"""
This script forwards your local UNIX mail to a given remote address using the
parameters you specify. You can accomplish the same thing with a local MTA, but
you will almost certainly run into difficulties if your machine is not
externally-facing and does not have an FQDN.

All code in this script is in the public domain. You may use it for any purpose
(including commercial) with or without attribution to the original author.

There are some current limitations including authentication support only up to
TLS and no support for local mailboxes in a format other than mbox. If you need
this changed or would like some other feature implemented ping me and I'll
probably do it.

Matthew Fernandez <matthew.fernandez@gmail.com>
"""

import sys
import getpass
import mailbox
import smtplib
import socket
import argparse

# The default place to look for your local mail.
DEFAULT_MBOX = '/var/mail/%s' % getpass.getuser()

def main():
    global DEFAULT_MBOX

    # Parse command line arguments.
    parser = argparse.ArgumentParser('A script for forwarding local mail to another address.')
    parser.add_argument('-f', '--from_address', required=True, help='From address to use when forwarding')
    parser.add_argument('--login', help='Login name if authentication is required for sending')
    parser.add_argument('-m', '--mbox', default=DEFAULT_MBOX, help='Mbox file to open if not the default')
    parser.add_argument('-p', '--port', default=25, type=int, help='Port to send through if not 25')
    parser.add_argument('--password', help='Password if authentication is required for sending')
    parser.add_argument('-s', '--server', required=True, help='SMTP server to forward messages through')
    parser.add_argument('-t', '--to', required=True, help='Address to forward to')
    parser.add_argument('--tls', default=False, action='store_true', help='Use TLS security (default is False)')
    p = parser.parse_args()

    # FIXME: This is almost certainly not how argparse should be used, but I'm
    # in a hurry.
    if 'f' in p: p.from_address = p.f
    if 'm' in p: p.mbox = p.m
    if 'p' in p: p.port = p.p
    if 's' in p: p.server = p.s
    if 't' in p: p.to = p.t

    hostname = socket.gethostname()

    # Open the local mailbox.
    box = None
    try:
        box = mailbox.mbox(p.mbox)
    except Exception as inst:
        sys.stderr.write('Failed to open %s: %s\n' % (p.mbox, str(inst)))
        return 1

    # Connect to the SMTP server.
    smtp = None
    try:
        smtp = smtplib.SMTP()
        smtp.connect(p.server, p.port)
        if p.tls:
            smtp.starttls()
        if p.login:
            smtp.login(p.login, p.password)
    except Exception as inst:
        sys.stderr.write('Failed to connect to %s: %s\n' % \
            (p.server, str(inst)))
        return 1

    # Forward and delete each message.
    box.lock()
    for msg in box.items():
        try:
            smtp.sendmail(p.from_address, p.to, \
"""From: %(from)s
To: %(to)s
Subject: %(hostname)s: %(subject)s

Forwarded email from %(hostname)s:%(mailbox)s:

%(message)s""" % {\
                'from':p.from_address, \
                'to':p.to, \
                'hostname':hostname, \
                'subject':msg[1]['Subject'] or '', \
                'mailbox':p.mbox, \
                'message':str(msg[1]) or ''})
            box.remove(msg[0])
        except Exception as inst:
            sys.stderr.write('Failed to send/delete message %d: %s\n' % \
                (msg[0], str(inst)))
            box.flush()
            box.unlock()
            return 1
    box.flush()
    box.unlock()

    return 0

if __name__ == '__main__':
    sys.exit(main())
