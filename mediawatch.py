#!/usr/bin/env python

"""
This script scans a given list of directories and notifies a list of users by
email of any changes observed since the previous scan. It is designed to be run
as a cron job. Note that there is no error handling so you will need to check
your cron mail to ensure the script is running correctly.
"""

import os.path
import smtplib
import socket
import sys
import getopt
import hashlib

NEW_FILE = 0
MODIFIED_FILE = 1
REMOVED_FILE = 2
UNCHANGED_FILE = 3

READ_BUFFER = 10240

def parseArguments(options):
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'd:f:p:s:t:', ['database=', \
            'from=', 'path=', 'server=', 'subject=', 'to='])
        if args:
            raise 'Extra arguments on command line'
        for o, a in opts:
            if o in ['-d', '--database']:
                options['database'] = a
            elif o in ['-f', '--from']:
                options['from'] = a
            elif o in ['-p', '--path']:
                options['paths'].append(a)
            elif o in ['-s', '--server']:
                options['server'] = a
            elif o == '--subject':
                options['subject'] = a
            elif o in ['-t', '--to']:
                options['to'].append(a)
            else:
                raise 'Unexpected argument'
    except:
        return False
    return True

def getHash(filepath):
    try:
        hash = hashlib.md5()
        f = open(filepath, 'rb')
        data = f.read(READ_BUFFER)
        while data:
            hash.update(data)
            data = f.read(READ_BUFFER)
        f.close()
        return hash.hexdigest()
    except:
        # If we can't read the file, that's not a critical error.
        # FIXME: command line opts for these msgs.
        sys.stderr.write('Warning: failed to hash %s: %s\n' % \
            (filepath, sys.exc_info()[0]))
        return ''

def updateFileTable(table, path):
    try:
        for file in os.listdir(path):
            if file.startswith('.'):
                # Ignore hidden files. TODO: Make this a command line opt.
                continue
            file = os.path.join(path, file)
            if os.path.isdir(file):
                updateFileTable(table, file)
            elif file in table:
                if str(os.path.getmtime(file)) == table[file]['modified']:
                    table[file]['state'] = UNCHANGED_FILE
                else:
                    table[file]['state'] = MODIFIED_FILE
                    table[file]['modified'] = str(os.path.getmtime(file))
                    table[file]['hash'] = getHash(file)
            else:
                table[file] = {'path':file, \
                               'modified':str(os.path.getmtime(file)), \
                               'hash':getHash(file), \
                               'state':NEW_FILE}
    except:
        sys.stderr.write('Warning: failed reading from directory %s: %s\n' % \
            (path, sys.exc_info()[0]))

def main():
    # Setup default options.
    options = {
        'database':None,
        'paths':[],
        'subject':'File changes on %s' % socket.gethostname(),
        'to':[],
        'from':None,
        'server':None,
    }

    if not parseArguments(options) or \
       not options['database'] or \
       not options['paths'] or \
       not options['to'] or \
       not options['from'] or \
       not options['server']:
        sys.stderr.write(\
"""Usage: %s options
 Options:
  -d file | --database=file   File to read/write file information from/to, recording data from last time and this pass. This option is required.
  -f address | --from=address Address to send from. This option is required.
  -p path | --path=path       A path to examine. This option must be used at least once, but can also be used multiple times.
  -s server | --server=server SMTP server to send through. This option is required.
  --subject=subject           Subject for the notification email. This is optional.
  -t address | --to=address   Address to send to. This option must be used at least once, but can also be used multiple times.
""" % sys.argv[0])
        return -1

    # A dictionary of files keyed on the path.
    files = {}

    if os.path.exists(options['database']):
        # If the database doesn't exist, assume this is the first time this has
        # been run.
        try:
            f = open(options['database'], 'r')
            for line in f:
                if not line.strip() or line.startswith('#'):
                    # Allow comments and blank lines.
                    continue
                d = dict(zip(['hash', 'path', 'modified'], \
                    line.strip().split('|')))
                d['state'] = REMOVED_FILE
                files[d['path']] = d
            f.close()
        except:
            sys.stderr.write('Error loading database %s: %s\n' % \
                (options['database'], sys.exc_info()[0]))
            return -1

    for p in options['paths']:
        updateFileTable(files, p)

    # Construct email
    message = \
"""From: Media Watch <%(from)s>
To: %(to)s
Subject: %(subject)s

The following changes have been recorded on your server. + indicates an added file, - a removed file and M a modified file.

""" % {'from':options['from'], \
       'to':', '.join(options['to']), \
       'subject':options['subject']}
    sendEmail = 0

    try:
        f = open(options['database'], 'w')
        for key in sorted(files.keys()):
            if files[key]['state'] != REMOVED_FILE:
                f.write('%s|%s|%s\n' % (files[key]['hash'], key,
                    files[key]['modified']))
            if files[key]['state'] == NEW_FILE:
                message += '+ %s\n' % key
                sendEmail = 1
            elif files[key]['state'] == MODIFIED_FILE:
                message += 'M %s\n' % key
                sendEmail = 1
            elif files[key]['state'] == REMOVED_FILE:
                message += '- %s\n' % key
                sendEmail = 1
        f.close()
    except:
        sys.stderr.write('Failed to write to database %s: %s.\n' % \
            (options['database'], sys.exc_info()[0]))
        return -1

    if sendEmail:
        try:
            smtpObj = smtplib.SMTP(options['server'])
            smtpObj.sendmail(options['from'], options['to'], message)
            smtpObj.quit()
        except:
            sys.stderr.write('Failed to send notification email: %s.\n' % \
                sys.exc_info()[0])
            return -1
    return 0

if __name__ == '__main__':
    sys.exit(main())
