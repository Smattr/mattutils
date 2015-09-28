#!/usr/bin/env python

'''
Minimum viable build system
'''

import argparse, collections, hashlib, json, os, shelve, six, subprocess, sys

class MVBSError(Exception):
    pass

def signature(path):
    if not os.path.exists(path):
        return None
    with open(path, 'rb') as f:
        return hashlib.sha256(f.read()).hexdigest()

class Rule(object):
    def __init__(self, target, dependencies, actions):
        if not isinstance(target, six.string_types):
            raise ValueError('target is not a string')
        if not isinstance(dependencies, list) or \
                not all(isinstance(x, six.string_types) for x in dependencies):
            raise ValueError('dependencies is not a list of strings')
        if not isinstance(actions, list) or \
                not all(isinstance(x, six.string_types) for x in actions):
            raise ValueError('actions is not a list of strings')
        self.target = target
        self.dependencies = dependencies
        self.actions = actions

    def run(self, verbose):
        assert isinstance(verbose, bool)
        for a in self.actions:
            if verbose:
                sys.stdout.write('+ %s\n' % a)
            ret = subprocess.call(a, shell=True)
            if ret != 0:
                raise MVBSError('failed to build %s' % self.target)

class RuleSet(object):
    def __init__(self):
        self.default = None
        self.rules = {}

    def add(self, rule):
        if rule.target in self.rules:
            raise MVBSError('duplicate rules for target \'%s\'' % rule.target)
        if len(self.rules) == 0:
            self.default = rule.target
        self.rules[rule.target] = rule

    def build(self, target, verbose, sigs):
        try:
            r = self.rules[target]
        except KeyError:
            if not os.path.exists(target):
                raise MVBSError('no rule to build target \'%s\'' % target)

        rebuild = not os.path.exists(target)

        for d in r.dependencies:
            self.build(d, verbose, sigs)
            s = signature(d)
            if s is None:
                raise MVBSError('recipe for target \'%s\' did not build it' % d)
            # XXX
            if isinstance(d, unicode):
                d = d.encode('utf-8')
            if d not in sigs or s != sigs[d]:
                sigs[d] = s
                rebuild = True

        if rebuild:
            r.run(verbose)
        elif verbose:
            sys.stdout.write('%s does not need to be rebuilt\n' % target)

def expand(path, root):
    return os.path.join(root, os.path.expanduser(os.path.expandvars(path)))

def parse(filename):

    filename = os.path.abspath(filename)

    rules = RuleSet()
    root = os.path.dirname(filename)

    with open(filename, 'rt') as f:
        try:
            data = json.load(f)
        except ValueError as e:
            raise MVBSError('failed to parse build file: %s' % str(e))

    if not isinstance(data, list):
        raise MVBSError('build file does not contain a list as expected')

    for d in data:
        try:
            target = expand(d['target'], root)
            dependencies = [expand(x, root) for x in d['dependencies']]
            actions = d['actions']
            rules.add(Rule(target, dependencies, actions))
        except (KeyError, ValueError, TypeError) as e:
            raise MVBSError('malformed recipe: %s' % d)

    return rules

def main(argv):
    parser = argparse.ArgumentParser(description='Minimum viable build system')
    parser.add_argument('--file', '--makefile', '-f',
        type=argparse.FileType('r'), help='Makefile to read')
    parser.add_argument('--verbose', action='store_true', help='be verbose')
    parser.add_argument('target', nargs='?', help='Target to build')
    options = parser.parse_args(argv[1:])

    if options.file is None:
        try:
            options.file = open('mvbs.json', 'r')
        except IOError:
            raise MVBSError('no makefile found')

    rules = parse(os.path.abspath(options.file.name))

    if rules.default is None:
        raise MVBSError('no rules defined')

    if options.target is None:
        options.target = rules.default

    try:
        sigs = shelve.open(os.path.expanduser('~/.mvbs.signatures.shelve'))

        rules.build(options.target, options.verbose, sigs)

    except MVBSError:
        sigs.close()
        raise

    return 0

if __name__ == '__main__':
    try:
        sys.exit(main(sys.argv))
    except MVBSError as e:
        sys.stderr.write('%s\n' % str(e))
        sys.exit(-1)
    except KeyboardInterrupt:
        sys.exit(130)
