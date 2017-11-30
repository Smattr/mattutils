#!/usr/bin/env python3

"""
This script forwards your local UNIX mail to a given remote address using the
parameters you specify. You can accomplish the same thing with a local MTA, but
you will almost certainly run into difficulties if your machine is not
externally-facing and does not have an FQDN.

Getting local mail working at all seems to be a fiddly process. A
non-exhaustive list of things I often need to do is:
 - Configure postfix (or whatever mail server you're using) to deliver local
   mail. sudo dpkg-reconfigure postfix if necessary.
 - sudo usermod -aG mail ${USER}
 - sudo touch /var/mail/${USER} && sudo chown ${USER} /var/mail/${USER}

All code in this script is in the public domain. You may use it for any purpose
(including commercial) with or without attribution to the original author.

There are some current limitations including authentication support only up to
TLS and no support for local mailboxes in a format other than mbox. If you need
this changed or would like some other feature implemented ping me and I'll
probably do it.

Matthew Fernandez <matthew.fernandez@gmail.com>
"""

import argparse, functools, getpass, grp, mailbox, os, smtplib, socket, sys, \
    syslog

def main(argv, stdout, stderr):
    # Parse command line arguments.
    parser = argparse.ArgumentParser(prog='fwdmail.py',
        description='Forward local mail to another address')
    parser.add_argument('--check_connection', action='store_true',
        help='Exit with success if offline')
    parser.add_argument('--from_address', '-f', required=True,
        help='From address to use when forwarding')
    parser.add_argument('--login',
        help='Login name if authentication is required for sending')
    parser.add_argument('--mbox', '-m', default='/var/mail/%s' % getpass.getuser(),
        help='Mbox file to open if not the default')
    parser.add_argument('--port', '-p', default=25, type=int,
        help='Port to send through')
    parser.add_argument('--password',
        help='Password if authentication is required for sending')
    parser.add_argument('--server', '-s', required=True,
        help='SMTP server to forward messages through')
    parser.add_argument('--to', '-t', required=True,
        help='Address to forward to')
    parser.add_argument('--tls', action='store_true',
        help='Use TLS security')

    p = parser.parse_args(argv[1:])

    hostname = socket.gethostname()

    # Check we're part of expected groups.
    for gid in os.getgroups():
        if grp.getgrgid(gid).gr_name == 'mail':
            break
    else:
        stderr('Warning: you are not part of the \'mail\' group')

    # Open the local mailbox.
    box = None
    try:
        box = mailbox.mbox(p.mbox)
    except Exception as inst:
        stderr('Failed to open %s: %s' % (p.mbox, inst))
        return 1

    smtp = None

    # Forward and delete each message.
    try:
        box.lock()
    except Exception as inst:
        stderr('Failed to lock mailbox file: %s' % inst)
        return -1
    for msg in box.items():
        if not smtp:
            # Connect to the SMTP server.
            try:
                try:
                    smtp = smtplib.SMTP()
                    smtp.connect(p.server, p.port)
                except Exception as inst:
                    if p.check_connection:
                        box.flush()
                        box.unlock()
                        return 0
                    else:
                        raise inst
                if p.tls:
                    smtp.starttls()
                if p.login:
                    smtp.login(p.login, p.password)
            except Exception as inst:
                stderr('Failed to connect to %s: %s' % (p.server, inst))
                box.flush()
                box.unlock()
                return 1
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
            stderr('Failed to send/delete message %d: %s' % (msg[0], inst))
            try:
                smtp.quit()
            except:
                pass # Ignore.
            box.flush()
            box.unlock()
            return -1
    if smtp: smtp.quit()
    box.flush()
    box.unlock()

    return 0

if __name__ == '__main__':
    if sys.stdout.isatty():
        def stdout(msg):
            sys.stdout.write('%s\n' % msg)
        def stderr(msg):
            sys.stderr.write('%s\n' % msg)
    else:
        # We seem to be running from a crontab.
        syslog.openlog('fwdmail', syslog.LOG_PID, syslog.LOG_MAIL)
        stdout = syslog.syslog
        stderr = functools.partial(syslog.syslog, syslog.LOG_ERR)
    sys.exit(main(sys.argv, stdout, stderr))
