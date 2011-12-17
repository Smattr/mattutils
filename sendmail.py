#!/usr/bin/env python

"""
This script provides the ability to send mail from the command line.
 - Matthew Fernandez <matthew.fernandez@gmail.com>

While there are existing Linux utilities to do this, they typically involve a
complete mail server (or at least MTA) configured. When you just want to send
a one-off email without configuring an email client or need to script
notification emails, this script can be handy. Note that it has limited support
for email features (e.g. no CC/BCC options). If you need extra features, email
me and I'll probably be happy to add them.

Run with no arguments to see valid options.
"""

import sys

# Import library for sending email.
try:
    import smtplib
except ImportError:
    sys.stderr.write('Failed to import smtplib. Is it installed?\n')
    sys.exit(1)

# Import library for parsing command line options.
try:
    import getopt
except ImportError:
    sys.stderr.write('Failed to import getopt. Is it installed?\n')
    sys.exit(1)


"""
Parse command line options.

 @param options A default dictionary of settings.
 @return True on success, False on failure.
"""
def parseArguments(options):
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'f:h:l:p:t:s:',
            ['from=', 'host=', 'login=', 'password=', 'port=', 'tls', 'to=',
            'subject='])
        if args:
            raise 'Extra arguments on command line'
        for o, a in opts:
            if o == '--empty':
                options['empty-cancel'] = True
            if o in ['-f', '--from']:
                options['from'] = a
            elif o in ['-h', '--host']:
                options['host'] = a
            elif o in ['-l', '--login']:
                options['login'] = a
            elif o in ['-p', '--password']:
                options['password'] = a
            elif o == '--port':
                options['port'] = int(a)
            elif o == '--tls':
                options['tls'] = True
            elif o in ['-t', '--to']:
                options['to'].append(a)
            elif o in ['-s', '--subject']:
                options['subject'] = a
            else:
                raise 'Unexpected argument'
    except:
        return False
    return True

def main():
    # Setup some default options.
    options = {
        'empty-cancel':False,
        'from':None,
        'host':None,
        'login':None,
        'password':None,
        'port':25,
        'tls':False,
        'to':[],
        'subject':None,
    }

    # Parse the command line options.
    if not parseArguments(options) or \
        not options['from'] or \
        not options['host'] or \
        not options['to']:
        sys.stderr.write("""Usage: %(prog)s options
Reads an email from STDIN with standard SMTP headers and sends it using the
parameters provided on the command line.
 [ --empty ]                           Don't send email and quit with success
                                       if an empty body is supplied.
 [ -f address | --from=address ]       Sender's address.
 [ -h hostname | --host=hostname ]     SMTP server.
 [ -l login | --login=login ]          Username.
 [ -p password | --password=password ] Password.
 [ --port=port ]                       SMTP port.
 [ --tls ]                             Use TLS security.
 [ -t address | --to=address ]         Add a recipient.
 [ -s subject | --subject=subject ]    Message subject.
""" % {'prog':sys.argv[0]})
        return -1

    # Read the message body. It's possible to do this inline below when
    # actually sending the email, but if there is any delay in reading the
    # input (e.g. if the user is calling this script interactively) the
    # connection could timeout.
    message = sys.stdin.read()
    if options['empty-cancel'] and not message:
        return 0

    # Send the email.
    try:
        smtpObj = smtplib.SMTP(options['host'], options['port'])
        if options['tls']:
            smtpObj.starttls()
        if options['login']:
            smtpObj.login(options['login'], options['password'])
        smtpObj.sendmail(options['from'], options['to'], message)
        smtpObj.quit()
    except:
        sys.stderr.write('Failed while sending: %s.\n' % str(sys.exc_info()[0]))
        return -1

    return 0

if __name__ == '__main__':
    sys.exit(main())
