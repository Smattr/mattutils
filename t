#!/usr/bin/env bash

# Rejoin a Tmux session, or start a new one if there is no current one.

if ! which tmux &>/dev/null; then
  printf 'tmux not found\n' >&2
  exit 1
fi

if tmux list-sessions &>/dev/null; then
  exec ssh-agent tmux attach
else
  exec ssh-agent tmux
fi
