#!/usr/bin/env bash

# Rejoin a Tmux session, or start a new one if there is no current one.

tmux list-sessions &>/dev/null
if [ $? -eq 0 ]; then
    exec ssh-agent tmux attach
else
    exec ssh-agent tmux
fi
