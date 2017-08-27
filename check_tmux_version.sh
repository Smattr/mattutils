#!/bin/bash

# Check if we're running Tmux ≥1.9 and enable some more advanced features that
# only work after 1.9. We need to do this in a separate shell script rather
# than Tmux's if-shell, because Tmux mangles some of the conditional logic.

TMUX_VERSION=$(tmux -V | cut -d' ' -f 2)

if [ $(awk 'BEGIN{ print "'${TMUX_VERSION}'">="1.9" }') -eq 1 ]; then
    tmux source-file ${HOME}/.tmux-modern.conf
else
    tmux source-file ${HOME}/.tmux-legacy.conf
fi
