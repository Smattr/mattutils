#!/usr/bin/env python3

'''
Script for preparing file(s) to send to someone else.

I often need to email/mail files to people when encryption and compression are
nice to apply by default. Although there are off-the-shelf tools that will
compress and encrypt files in one shot, all of them seem lacking in one way or
another (inefficient compression, weak encryption, single threaded, ...), so I
typically fall back on command line tools. I find it challenging to remember
the cryptic set of options you need to pass these tools, so this script
automates that part of it.

Note, OpenSSL encryption is available as an alternative to GPG because Mac
recipients typically don't have GPG available by default.
'''

import argparse, getpass, os, subprocess, sys

def which(command):
    try:
        with open(os.devnull, 'w') as f:
            return subprocess.check_output(['which', command], stderr=f,
                universal_newlines=True).strip()
    except subprocess.CalledProcessError:
        return None

def main(argv):
    parser = argparse.ArgumentParser(
        description='package a file or directory for sending')
    parser.add_argument('--input', '-i', help='input file or directory',
        required=True)
    parser.add_argument('--compress', action='store_true', default=True,
        help='compress package')
    parser.add_argument('--no-compress', action='store_false',
        dest='compress', help='do not compress package')
    parser.add_argument('--progress', action='store_true', default=True,
        help='show progress')
    parser.add_argument('--no-progress', action='store_false',
        dest='progress', help='do not show progress')
    parser.add_argument('--gpg-encrypt', action='store_true', default=True,
        help='encrypt with GPG')
    parser.add_argument('--no-gpg-encrypt', action='store_false',
        dest='gpg_encrypt', help='do not encrypt with GPG')
    parser.add_argument('--openssl-encrypt', action='store_true',
        help='encrypt with OpenSSL')
    parser.add_argument('--no-openssl-encrypt', action='store_false',
        dest='openssl_encrypt', help='do not encrypt with OpenSSL')
    parser.add_argument('--split', type=int, help='split at SPLIT bytes')
    parser.add_argument('--output', '-o', help='output file')
    options = parser.parse_args(argv[1:])

    if not os.path.exists(options.input):
        sys.stderr.write('%s not found\n' % options.input)
        return -1

    commands = ['tar']
    if options.compress:
        commands.append('7za')
    if options.gpg_encrypt:
        commands.append('gpg')
    if options.openssl_encrypt:
        commands.append('openssl')
    if options.progress:
        commands.append('pv')
    if options.split is not None:
        commands.append('split')

    for c in commands:
        if which(c) is None:
            sys.stderr.write('%s not found\n' % c)
            return -1

    if options.output is None:
        output = '%s.tar' % os.path.abspath(options.input)
        if options.compress:
            output += '.xz'
        if options.gpg_encrypt:
            output += '.gpg'
        if options.openssl_encrypt:
            output += '.enc'
    else:
        output = os.path.abspath(options.output)
    if options.split is not None:
        output += '.'

    if not os.path.exists(os.path.dirname(output)):
        sys.stderr.write('containing directory %s for output does not exist\n' %
            os.path.dirname(output))
        return -1

    if options.split is None:
        if os.path.exists(output):
            sys.stderr.write('%s already exists\n' % output)
            return -1
    else:
        prefix = os.path.basename(output)
        for f in os.listdir(os.path.dirname(output)):
            if f.startswith(prefix):
                sys.stderr.write('existing file %s might be overwritten\n' %
                    os.path.join(os.path.dirname(output), f))
                return -1

    if options.gpg_encrypt or options.openssl_encrypt:
        password = getpass.getpass('password: ')
        confirm = getpass.getpass('confirm password: ')
        if password != confirm:
            sys.stderr.write('passwords do not match\n')
            return -1
        if '\'' in password:
            sys.stderr.write('passwords cannot contain \'\n')
            return -1

    if os.path.isfile(options.input):
        pipeline = ['tar cp \'%s\'' % os.path.basename(options.input)]
        cwd = os.path.dirname(os.path.abspath(options.input))
    else:
        pipeline = ['tar cp *']
        cwd = options.input
    if options.compress:
        pipeline.append('7za a -an -txz -m0=LZMA2 -si -so 2>/dev/null')
    if options.gpg_encrypt:
        pipeline.append('gpg -c --no-use-agent --passphrase \'%s\'' % password)
    if options.openssl_encrypt:
        pipeline.append('openssl enc -aes-256-cbc -k \'%s\'' % password)
    if options.progress:
        pipeline.append('pv')

    if options.split is None:
        try:
            with open(output, 'wb') as f:
                subprocess.check_call('|'.join(pipeline), shell=True, cwd=cwd,
                    stdout=f)
        except subprocess.CalledProcessError as e:
            sys.stderr.write('failed: %s\n' % str(e))
            return -1
    else:
        pipeline.append('split --bytes=%d --numeric-suffixes - \'%s\'' %
            (options.split, output))
        try:
            subprocess.check_call('|'.join(pipeline), shell=True, cwd=cwd)
        except subprocess.CalledProcessError as e:
            sys.stderr.write('failed: %s\n' % str(e))
            return -1

    sys.stdout.write('done\n'
                     'to decrypt: cat %s%s' % (os.path.basename(output),
        '' if options.split is None else '*'))
    if options.openssl_encrypt:
        sys.stdout.write(' | openssl enc -aes-256-cbc -d')
    if options.gpg_encrypt:
        sys.stdout.write(' | gpg --decrypt --no-use-agent')
    sys.stdout.write(' | tar x')
    if options.compress:
        sys.stdout.write('J')
    sys.stdout.write('\n')

    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv))
