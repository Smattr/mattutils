#!/bin/bash

# Rejoin a Tmux session, or start a new one if there is no current one.

tmux list-sessions &>/dev/null
if [ $? -eq 0 ]; then
    ssh-agent tmux attach
else
    ssh-agent tmux
fi
