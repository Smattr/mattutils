#!/usr/bin/env python

'''
Choose a random system user. This also tracks the number of times they have
'won' the lottery in the past and makes the likelihood of their winning future
lotteries the inverse of their score. The idea is that you are choosing a user
for some onerous task so nobody wants to win.
    
Code should be fairly self-explanatory, if not sensible. Feel free to use for
anything, commercial or otherwise, but don't blame me when you hit bugs.
'''

import getopt, pwd, random, sys

# Default file to store previous scores in.
DEFAULT_DB = '.lottery.txt'

# Starting score for a newly discovered user.
STARTING_SCORE = 1

class Scores(object):
    def __init__(self, filename):
        assert isinstance(filename, str)
        self.filename = filename
        self._users = {}
        try:
            with open(filename, 'r') as f:
                for line in f.readlines():
                    try:
                        user, score = line.split(':')
                        self._users[user] = int(score)
                    except:
                        pass
        except:
            pass

    def merge(self, users):
        '''
        Add any users from the passed list that do not already have an
        associated score.
        '''
        for u in users:
            if u not in self._users:
                self._users[u] = STARTING_SCORE

    def choose(self, excludes):
        '''
        Choose a random user. This function is full of inefficiencies.
        '''
        # Filter out users we're not including in this choice.
        users = {u: self._users[u] for u in (set(self._users.keys()) - set(excludes))}
        assert 0 not in users.values()
        assert len(users) != 0

        # Get a common multiple of the users' scores.
        cm = reduce(lambda x, y: x * y, users.values(), 1)

        # Generate a list of users with each users' name appearing a number of
        # times inverse to their score.
        seq = [i for ys in [[u] * (cm / users[u]) for u in users] for i in ys]

        # Choose a lucky winner.
        u = random.choice(seq)
        self._users[u] += 1
        return u

    def __getitem__(self, user):
        return self._users[user]

    def __iter__(self):
        return self._users.__iter__()

    def __del__(self):
        '''
        Write out scores back to our backing file when exiting. Ignore errors.
        '''
        try:
            with open(self.filename, 'w') as f:
                for user in self._users:
                    print >>f, '%s:%d' % (user, self._users[user])
        except:
            pass
        

USAGE = '''Usage: %(argv0)s options
  Options:
   [ -f FILENAME | --filename=FILENAME ]  Use FILENAME as score database (default %(db)s).
   [ -x NAME | --exclude NAME ]           Exclude user NAME from the lottery.
   [ -? | --help ]                        Print this information.
''' % {'argv0':sys.argv[0], 'db':DEFAULT_DB}

def parse_arguments(options):
    opts, args = getopt.getopt(sys.argv[1:], 'f:x:?', ['filename=', 'exclude=', 'help'])
    if args:
        raise Exception('Unrecognised options: %s' % ' '.join(args))
    for o, a in opts:
        if o in ['-f', '--filename']:
            options['filename'] = a
        elif o in ['-x', '--exclude']:
            options['exclude'].append(a)
        elif o in ['-?', '--help']:
            print USAGE
            sys.exit(0)

def get_users():
    '''
    Return a list of all non-system users. 'Non-system' on my machine means,
    uid >= 1000, but this can/may/will be different for you.
    '''
    return map(lambda x: x.pw_name, \
            filter(lambda x: x.pw_uid >= 1000, pwd.getpwall()))

def main():
    options = {
        'filename':DEFAULT_DB,
        'exclude':[],
    }

    try:
        parse_arguments(options)
    except inst as Exception:
        print >>sys.stderr, 'While parsing arguments: %s' % str(inst)
        return -1

    scores = Scores(options['filename'])
    scores.merge(get_users())

    print scores.choose(options['exclude'])
    return 0

if __name__ == '__main__':
    sys.exit(main())
