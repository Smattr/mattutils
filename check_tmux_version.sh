#!/usr/bin/env bash

# Check what version of Tmux we're running to enable various compatibility
# tweaks. We need to do this in a separate shell script rather than Tmux's
# if-shell, because Tmux mangles some of the conditional logic.

export TMUX_BIN="${TMUX_BIN:-tmux}"

TMUX_VERSION=$(${TMUX_BIN} -V | cut -d' ' -f 2)

if [ $(awk 'BEGIN{ print ("'${TMUX_VERSION}'">="1.9") }') -eq 1 ]; then
    tmux source-file ${HOME}/.tmux-new-pane_current_path.conf
else
    tmux source-file ${HOME}/.tmux-old-pane_current_path.conf
fi

if [ $(awk 'BEGIN{ print ("'${TMUX_VERSION}'">="2.4") }') -eq 1 ]; then
    tmux source-file ${HOME}/.tmux-new-vi-copy.conf
else
    tmux source-file ${HOME}/.tmux-old-vi-copy.conf
fi

if [ $(awk 'BEGIN{ print ("'${TMUX_VERSION}'">="1.9") }') -eq 1 ]; then
    tmux source-file ${HOME}/.tmux-tpm.conf
fi
