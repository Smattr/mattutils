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
import optparse

# The default place to look for your local mail.
DEFAULT_MBOX = '/var/mail/%s' % getpass.getuser()

# The usage information string.
USAGE = """Usage: %(prog)s options
  Forward local mail to another address.
 [-f address | --from_address address]  Address to send from.
 [-t address | --to address]            Address to send to.
 [-s server | --server server]          SMTP server to forward through.
 [-p port | --port port]                Port to connect to (default 25).
 [-m file | --mbox file]                Use a specific mailbox (default %(mbox)s).
 [--login login]                        Login name for SMTP if required.
 [--password password]                  Password for SMTP if required.
 [--tls]                                Use TLS security (default off).
"""

def main():
    global DEFAULT_MBOX
    global USAGE

    # Parse command line arguments.
    parser = optparse.OptionParser()
    parser.add_option('-f', '--from_address', \
                      dest='from_address', \
                      help='From address to use when forwarding')
    parser.add_option('--login', \
                      dest='login', \
                      help='Login name if authentication is required for sending')
    parser.add_option('-m', '--mbox', \
                      dest='mbox', \
                      default=DEFAULT_MBOX, \
                      help='Mbox file to open if not the default')
    parser.add_option('-p', '--port', \
                      dest='port', \
                      default=25, \
                      type=int, \
                      help='Port to send through if not 25')
    parser.add_option('--password', \
                      dest='password', \
                      help='Password if authentication is required for sending')
    parser.add_option('-s', '--server', \
                      dest='server', \
                      help='SMTP server to forward messages through')
    parser.add_option('-t', '--to', \
                      dest='to', \
                      help='Address to forward to')
    parser.add_option('--tls', \
                      dest='tls', \
                      default=False, \
                      action='store_true', \
                      help='Use TLS security (default is False)')
    p = None
    args = None
    try:
        (p, args) = parser.parse_args()
        if args or not p.from_address \
                or not p.to \
                or not p.server:
            raise Exception('Illegal arguments')
    except Exception as inst:
        print str(inst)
        sys.stderr.write(USAGE % {'prog':sys.argv[0], \
                                  'mbox':DEFAULT_MBOX})
        return -1

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
    try:
        box.lock()
    except Exception as inst:
        sys.stderr.write('Failed to lock mailbox file: %s\n' % str(inst))
        return -1
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
