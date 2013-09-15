#!/usr/bin/env python

"""
This script provides the ability to send mail from the command line.
 - Matthew Fernandez <matthew.fernandez@gmail.com>

While there are existing Linux utilities to do this, they typically involve a
complete mail server (or at least MTA) configured. When you just want to send
a one-off email without configuring an email client or need to script
notification emails, this script can be handy. If you need extra features, email
me and I'll probably be happy to add them.

Run with no arguments to see valid options.
"""

import argparse, smtplib, sys
from email.mime.text import MIMEText

def main():
    options = argparse.ArgumentParser(description='Reads an email body from ' + \
        'STDIN and sends it using the parameters provided on the command line.')
    options.add_argument('--bcc', '-b', action='append', default=[],
        help='add a BCC recipient')
    options.add_argument('--cc', '-c', action='append', default=[],
        help='add a CC recipient')
    options.add_argument('--date',
        help='set date header of the mail')
    options.add_argument('--debug', action='store_true',
        help='print the body that would be sent and exit without sending')
    options.add_argument('--empty', action='store_true',
        help='don\'t send email and quit with success if an empty body is supplied')
    options.add_argument('--from', '-f', required=True,
        help='sender\'s address')
    options.add_argument('--host', required=True,
        help='SMTP server')
    options.add_argument('--login', '-l',
        help='username')
    options.add_argument('--password', '-p',
        help='password')
    options.add_argument('--port', type=int, default=25,
        help='SMTP port')
    options.add_argument('--tls', action='store_true',
        help='use TLS security')
    options.add_argument('--to', '-t', action='append', default=[], required=True,
        help='add a recipient')
    options.add_argument('--subject', '-s',
        help='message subject')
    args = options.parse_args()

    # Read the message body. It's possible to do this inline below when
    # actually sending the email, but if there is any delay in reading the
    # input (e.g. if the user is calling this script interactively) the
    # connection could timeout.
    message = sys.stdin.read()
    if args.empty and not message:
        return 0
    message = MIMEText(message, 'plain', _charset='utf-8')
    message['To'] = ', '.join(args.to)
    message['From'] = args.__dict__['from']
    message['CC'] = ', '.join(args.cc)
    message['Subject'] = args.subject or ''
    if args.date:
        message['Date'] = args.date

    # If we're in debugging mode, bail out without sending the email.
    if args.debug:
        sys.stdout.write('%s\n' % message)
        return 0

    # Send the email.
    try:
        smtpObj = smtplib.SMTP(args.host, args.port)
        if args.tls:
            smtpObj.starttls()
        if args.login:
            smtpObj.login(args.login, args.password)
        smtpObj.sendmail(args.__dict__['from'], \
            args.to + args.cc + args.bcc, message.as_string())
        smtpObj.quit()
    except Exception as e:
        sys.stderr.write('Failed while sending: %s\n' % e)
        return -1

    return 0

if __name__ == '__main__':
    sys.exit(main())
