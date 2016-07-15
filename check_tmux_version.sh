#!/bin/bash

# Check if we're running Tmux ≥1.9 and run a work around for retaining the
# current working directory if necessary. We need to do this in a separate
# shell script rather than Tmux's if-shell, because Tmux mangles some of the
# conditional logic.

TMUX_VERSION=$(tmux -V | cut -d' ' -f 2)

if [ $(echo "${TMUX_VERSION} >= 1.9" | bc) -eq 1 ]; then
    tmux source-file ${HOME}/.tmux-1.9-workaround.conf
fi
