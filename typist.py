#!/usr/bin/env python3

'''
Tool for typing characters that aren't easy to access on a standard keyboard.
'''

import codecs
import json
import os
import platform
import subprocess
import sys

if platform.system() == 'Darwin' and not sys.stdout.isatty():
  # Assume we're running under Automator and we need to return success to
  # suppress its unhelpful generic error.
  FAIL = 0
else:
  FAIL = -1

if platform.system() == 'Darwin':
  def get_input() -> str:
    try:
      p = subprocess.run(['osascript'], input='text returned of (display '
        'dialog "ascii text?" default answer "" with title "typist")',
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True,
        encoding='utf-8')
    except subprocess.CalledProcessError:
      return None

    return p.stdout.strip()

  def show_error(s: str):
    subprocess.run(['osascript'], input='display dialog "{}" with title '
      '"typist" buttons "OK" default button 1 with icon stop'.format(s),
      check=True, encoding='utf-8')

  def escape(s: str) -> str:
    if s == '"' or s == '\\':
      return '\\{}'.format(s)
    return s

  def send_text(s: str):
    subprocess.run(['osascript'], input='set the clipboard to "{}"\n'
      'tell application "System Events" to keystroke "v" using command down'
      .format(''.join(escape(c) for c in s)),
      check=True, encoding='utf-8')

elif platform.system() == 'Linux':
  def get_input() -> str:
    try:
      p = subprocess.run(['zenity', '--entry', '--title', 'typist', '--text',
        'ascii text?'], stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        check=True, encoding='utf-8')
    except subprocess.CalledProcessError:
      return None

    return p.stdout.strip()

  def show_error(s: str):
    subprocess.run(['zenity', '--error', '--title', 'typist', '--text', s],
      check=True, encoding='utf-8')

  def send_text(s: str):
    for c in s:
      key = 'U{}'.format(hex(ord(c))[2:])
      subprocess.run(['xdotool', 'key', key], check=True, encoding='utf-8')


def main() -> int:

  config_path = os.path.expanduser('~/.typist.json')

  if not os.path.exists(config_path):
    show_error('configuration file not found')
    return FAIL

  # Load user's configuration. We deliberately don't bother checking errors here
  # to let the user debug from the stack trace.
  # XXX: macOS Automator uses some non-UTF-8 locale which presents a problem
  # when opening a UTF-8 file so we need to force it.
  with codecs.open(config_path, 'r', 'utf-8') as f:
    db = json.load(f)

  # Read some text the user wants translated.
  text = get_input()

  if text is None:
    # The user cancelled the dialog.
    return 0

  # Translate the user's input using their configuration file.
  output = []
  while len(text) > 0:
    # Here we repeatedly look at the longest unexamined prefix of the remaining
    # text.
    for l in reversed(range(len(text))):
      s = text[:l+1]
      if s in db:
        output.append(db[s])
        text = text[len(s):]
        break
    else:
      show_error('could not find a valid translation')
      return FAIL

  # Type it for the user.
  for o in output:
    send_text(o)

  return 0

if __name__ == '__main__':
  sys.exit(main())
