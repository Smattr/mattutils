#!/usr/bin/env bash

# Grep is indispensible for navigating large, dynamic code bases, but you often
# want a smarter, faster alternative. This script tries to find you such a thing
# and run it.

# Preference 1: Ripgrep
if which rg &>/dev/null; then
  exec rg "$@"
fi


# Preference 2: The Silver Searcher
if which ag &>/dev/null; then
  if ag --version &>/dev/null; then
    exec ag "$@"
  fi
fi

# Preference 3: Ack
if which ack &>/dev/null; then
  exec ack "$@"
fi

# Fallback: grep
exec find . 2>/dev/null -type f -print0 | xargs -0 -P $(getconf _NPROCESSORS_ONLN) grep --color=always -HI "$@" | less -iRnSFX
