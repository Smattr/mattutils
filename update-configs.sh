#!/bin/bash

# Update some configuration files that are constructed from multiple files.

# If we're running in certain contexts (e.g. crontab while logged out from
# system with encrypted homes) we want to bail out early.
if [ ! -d "${HOME}/.ssh" ]; then
    exit 0
fi

# SSH
cat ${HOME}/.ssh/config_* >${HOME}/.ssh/config

# Git
cat ${HOME}/.gitconfig_* >${HOME}/.gitconfig
