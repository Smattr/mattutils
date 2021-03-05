#!/usr/bin/env python3

'''
Tool for typing characters that aren't easy to access on a standard keyboard.
'''

import codecs
import logging
import os
from pathlib import Path
import platform
import re
import subprocess
import sys
from typing import Dict, Generator, Tuple

log = None

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

elif platform.system() == 'Windows':
  import ctypes
  import tkinter as tk
  from tkinter import simpledialog

  # suppress the TK default window
  root = tk.Tk()
  root.withdraw()

  def get_input() -> str:
    return simpledialog.askstring('typist', 'ascii text?')

  def show_error(s: str):
    MB_ICONERROR = 0x00000010
    ctypes.windll.user32.MessageBoxW(None, s, 'typist', MB_ICONERROR)

# keys that are named in Compose files
KEYS = {
  'apostrophe' :'\'',
  'asciicircum':'^',
  'asciitilde' :'~',
  'asterisk'   :'*',
  'backslash'  :'\\',
  'bar'        :'|',
  'braceleft'  :'{',
  'braceright' :'}',
  'colon'      :':',
  'comma'      :',',
  'equal'      :'=',
  'exclam'     :'!',
  'grave'      :'`',
  'greater'    :'>',
  'less'       :'<',
  'minus'      :'-',
  'numbersign' :'#',
  'parenleft'  :'(',
  'parenright' :')',
  'percent'    :'%',
  'period'     :'.',
  'plus'       :'+',
  'question'   :'?',
  'quotedbl'   :'"',
  'semicolon'  :';',
  'slash'      :'/',
  'space'      :' ',
  'underscore' :'_',
  # surprisingly @, $, &, [, ] all seem unused in en_US.UTF-8
}

def parse_compose(src: Path) -> Generator[Tuple[str, str], None, None]:
  '''
  parse an X Compose file
  '''

  # TODO: support include directives

  # XXX: macOS Automator uses some non-UTF-8 locale which presents a problem
  # when opening a UTF-8 file so we need to force it
  with codecs.open(src, 'r', 'utf-8') as f:
    for line in f:

      # skip comments
      if re.match(r'\s*#', line):
        log.debug(f'discarding comment line {line}')
        continue

      # skip events that are not initiated with the Compose key
      m = re.match(r'<Multi_key>\s+', line)
      if m is None:
        log.debug(f'discarding non-Multi_key line {line}')
        continue
      line = line[len(m.group(0)):]

      # parse the remaining events
      events = []
      while True:
        m = re.match(r'<([^>]+)>\s*', line)
        if m is None:
          break
        events.append(KEYS.get(m.group(1), m.group(1)))
        line = line[len(m.group(0)):]
      if len(events) == 0:
        log.debug(f'discarding event-free remainder {line}')
        continue

      for e in events:
        if len(e) > 1:
          # an unrecognised named character
          log.debug(f'unrecognised character "{e}"')

      # hope that this maps to a string
      m = re.match(r':\s*"([^"]+)"', line)
      if m is None:
        log.debug(f'discarding non-string mapping {line}')
        continue

      log.debug(f'found mapping {events} -> {m.group(1)}')
      yield ''.join(events), m.group(1)

def main() -> int:

  # setup logging
  global log
  log = logging.getLogger('deploy.py')
  log.setLevel(logging.DEBUG)
  # make it print to stderr if we are in a terminal
  if sys.stdout.isatty():
    ch = logging.StreamHandler()
    log.addHandler(ch)

  # mapping from key sequence to its replacement
  db: Dict[str, str] = {}

  # follow the Compose spec and look for (1) $XCOMPOSEFILE…
  if 'XCOMPOSEFILE' in os.environ:
    log.info(f'reading $XCOMPOSEFILE ({os.environ["XCOMPOSEFILE"]}) ...')
    db.update(parse_compose(Path(os.environ['XCOMPOSEFILE'])))

  # …or (2) ~/.XCompose…
  elif Path('~/.XCompose').expanduser().exists():
    log.info('reading ~/.XCompose ...')
    db.update(parse_compose(Path('~/.XCompose').expanduser()))

  # …or (3) the locale Compose file
  elif 'LANG' in os.environ:
    d = Path('/usr/share/X11/locale/compose.dir')
    if d.exists():
      log.info('reading /usr/share/X11/locale/compose.dir ...')
      c = None
      with codecs.open(d, 'r', 'utf-8') as f:
        for line in f:
          w = line.split()
          if len(w) == 2 and w[1] == os.environ['LANG']:
            c = Path(f'/usr/share/X11/locale/{w[0]}')
            break
      if c is not None:
        log.info(f'reading {c} ...')
        db.update(parse_compose(c))

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
