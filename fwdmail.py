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

import argparse
import contextlib
import functools
import getpass
import grp
import itertools
import mailbox
import os
import smtplib
import socket
import sys
import syslog
import time

@contextlib.contextmanager
def locked(mailbox):
  try:
    yield mailbox
  finally:
    mailbox.unlock()

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
    stderr(f'Failed to open {p.mbox}: {inst}')
    return 1

  smtp = None

  # Forward and delete each message.
  try:
    box.lock()
  except Exception as inst:
    stderr(f'Failed to lock mailbox file: {inst}')
    return -1
  with locked(box) as b:
    for i in itertools.count():

      try:
        key, msg = b.popitem()
      except KeyError:
        # mailbox empty
        break

      if not smtp:
        # Connect to the SMTP server.
        try:
          try:
            smtp = smtplib.SMTP(p.server, p.port)
            smtp.connect(p.server, p.port)
          except Exception as inst:
            if p.check_connection:
              return 0
            else:
              raise inst
          if p.tls:
            smtp.starttls()
          if p.login:
            smtp.login(p.login, p.password)
        except Exception as inst:
          stderr(f'Failed to connect to {p.server}: {inst}')
          return 1
      try:
        smtp.sendmail(p.from_address, p.to, \
  f"""From: {p.from_address}
  To: {p.to}
  Subject: {hostname}: {msg['Subject'] or ''}

  Forwarded email from {hostname}:{p.mbox}:

  {msg or ''}""".encode('utf-8', 'replace'))
        b.flush()
      except Exception as inst:
        stderr(f'Failed to send/delete message {key}: {inst}')
        try:
          smtp.quit()
        except:
          pass # Ignore.
        return -1
      # pause every 10 emails to avoid flooding the SMTP server
      if i % 10 == 9:
        time.sleep(0.5)
  if smtp: smtp.quit()

  return 0

if __name__ == '__main__':
  if sys.stdout.isatty():
    def stdout(msg):
      sys.stdout.write(f'{msg}\n')
    def stderr(msg):
      sys.stderr.write(f'{msg}\n')
  else:
    # We seem to be running from a crontab.
    syslog.openlog('fwdmail', syslog.LOG_PID, syslog.LOG_MAIL)
    stdout = syslog.syslog
    stderr = functools.partial(syslog.syslog, syslog.LOG_ERR)
  sys.exit(main(sys.argv, stdout, stderr))
