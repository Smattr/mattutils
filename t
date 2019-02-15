#!/usr/bin/env bash

# Rejoin a Tmux session, or start a new one if there is no current one.

export TMUX_BIN="${TMUX_BIN:-tmux}"

if ! which "${TMUX_BIN}" &>/dev/null; then
  printf 'tmux not found\n' >&2
  exit 1
fi

# Force Zsh if it's available. Useful in environments that don't let us change
# our default shell.
if which zsh &>/dev/null; then
  export SHELL=$(which zsh)
fi

if "${TMUX_BIN}" list-sessions &>/dev/null; then
  exec ssh-agent "${TMUX_BIN}" attach
else
  exec ssh-agent "${TMUX_BIN}"
fi
