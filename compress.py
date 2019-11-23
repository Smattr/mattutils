#!/usr/bin/env python3

'''
quick hack for compressing a file as small as possible
'''

import os
import stat
import subprocess
import sys
import time

def main(args):

  if len(args) != 2 or not os.path.isfile(args[1]):
    sys.stderr.write(f'usage: {args[0]} file_to_compress\n')
    return -1

  # assume we have ≥ 3 cores, and run everything in parallel
  bz = subprocess.Popen(['bzip2', '--keep', '-9', args[1]], stdout=subprocess.DEVNULL)
  gz = subprocess.Popen(['gzip', '--keep', '-9', args[1]], stdout=subprocess.DEVNULL)
  xz = subprocess.Popen(['xz', '--keep', '-9', args[1]], stdout=subprocess.DEVNULL)

  # wait for all processes to complete
  procs = [bz, gz, xz]
  while len(procs) > 0:
    i = 0
    while i < len(procs):
      if procs[i].poll() is not None:
        procs.pop(i)
      else:
        i += 1
    time.sleep(1)

  # find which compressed most
  winner = None
  winner_size = None
  for ext in ('.bz2', '.gz', '.xz'):
    candidate = f'{args[1]}{ext}'
    size = os.stat(candidate).st_size
    if winner_size is None or size < winner_size:
      if winner is not None:
        # remove a loser
        os.remove(winner)
      winner = candidate
      winner_size = size

  sys.stdout.write(f'winner is {candidate}\n')

  return 0

if __name__ == '__main__':
  sys.exit(main(sys.argv))
