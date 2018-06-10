#!/usr/bin/env bash

# Rejoin a Tmux session, or start a new one if there is no current one.

if ! which tmux &>/dev/null; then
  printf 'tmux not found\n' >&2
  exit 1
fi

# Force Zsh if it's available. Useful in environments that don't let us change
# our default shell.
if which zsh &>/dev/null; then
  export SHELL=$(which zsh)
fi

if tmux list-sessions &>/dev/null; then
  exec ssh-agent tmux attach
else
  exec ssh-agent tmux
fi
