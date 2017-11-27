#!/usr/bin/env bash

# Grep is indispensible for navigating large, dynamic code bases, but you often
# want a smarter, faster alternative. This script tries to find you such a thing
# and run it.

# Preference 1: Ripgrep
which rg &>/dev/null
if [ $? -eq 0 ]; then
  rg "$@"
else

  # Preference 2: The Silver Searcher
  which ag &>/dev/null
  if [ $? -eq 0 ]; then
    ag "$@"
  else

    # Preference 3: Ack
    which ack &>/dev/null
    if [ $? -eq 0 ]; then
      ack "$@"
    else

      # Fallback: grep
      find . 2>/dev/null -type f -exec grep --color=always -HI "$@" "{}" \; | less -iRnSFX

    fi
  fi
fi
