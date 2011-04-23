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

NEW_FILE = 0
MODIFIED_FILE = 1
REMOVED_FILE = 2
UNCHANGED_FILE = 3

# Store for previously observed files.
FILE_DB = '/home/mediawatch/MediaWatch.txt'

# Directories to scan.
PATHS = ['/mnt/Data/']

# Comma-separated list of addresses in the to field of the notification email.
HEADER_TO = 'Example User <example.user@example.com>, ' + \
            'Other User <other.user@example.com>'

# Persons to send the notification to. Should match HEADER_TO unless you're
# doing something odd.
RECEIVERS = ['example.user@example.com', \
             'other.user@example.com']

# From address for the notification email.
SENDER = 'admin@example.com'

# Subject of the notification email
SUBJECT = 'File changes on %s' % socket.gethostname()

# SMTP server to send through.
SERVER = 'mail.example.com'

##### Structures #####
class MediaFile:
    def __init__(self, modified):
        self.modified = modified
        self.state = REMOVED_FILE

##### Functions #####
def updateFileTable(table, path):
    for file in os.listdir(path):
        if file.startswith('.'):
            continue
        file = os.path.join(path, file)
        if os.path.isdir(file):
            updateFileTable(table, file)
        elif file in table:
            if str(os.path.getmtime(file)) == table[file].modified:
                table[file].state = UNCHANGED_FILE
            else:
                table[file].state = MODIFIED_FILE
                table[file].modified = str(os.path.getmtime(file))
        else:
            table[file] = MediaFile(str(os.path.getmtime(file)))
            table[file].state = NEW_FILE
    return

##### Logic #####
files = dict()

if os.path.exists(FILE_DB):
    f = open(FILE_DB, 'r')
    for line in f:
        attributes = line.split('|')
        files[attributes[0]] = MediaFile(attributes[1].strip())
    f.close()

for share in PATHS:
    updateFileTable(files, share)

# Construct email
message = 'From: Media Watch <' + SENDER + '>\n'
message += 'To: ' + HEADER_TO + '\n'
message += 'Subject: ' + SUBJECT + '\n'
message += 'The following changes have been recorded on your server. + indicates an added file, - a removed file and M a modified file.\n\n'
sendEmail = 0

f = open(FILE_DB, 'w')
for key in sorted(files.keys()):
    if files[key].state != REMOVED_FILE:
        f.write(key + '|' + files[key].modified + '\n')
    if files[key].state == NEW_FILE:
        message += '+ ' + key + '\n'
        sendEmail = 1
    elif files[key].state == MODIFIED_FILE:
        message += 'M ' + key + '\n'
        sendEmail = 1
    elif files[key].state == REMOVED_FILE:
        message += '- ' + key + '\n'
        sendEmail = 1
f.close()

if sendEmail:
    smtpObj = smtplib.SMTP(SERVER)
    smtpObj.sendmail(SENDER, RECEIVERS, message)
