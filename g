#!/usr/bin/env bash

# Grep is indispensible for navigating large, dynamic code bases, but you often
# want a smarter, faster alternative. This script tries to find you such a thing
# and run it.

# Preference 1: Ripgrep
which rg &>/dev/null
if [ $? -eq 0 ]; then
  exec rg "$@"
fi


# Preference 2: The Silver Searcher
which ag &>/dev/null
if [ $? -eq 0 ]; then
  ag --version &>/dev/null
  if [ $? -eq 0 ]; then
    exec ag "$@"
  fi
fi

# Preference 3: Ack
which ack &>/dev/null
if [ $? -eq 0 ]; then
  exec ack "$@"
fi

# Fallback: grep
exec find . 2>/dev/null -type f -exec grep --color=always -HI "$@" "{}" \; | less -iRnSFX
