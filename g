#!/usr/bin/env bash

# Grep is indispensible for navigating large, dynamic code bases, but you often
# want a smarter, faster alternative. This script tries to find you such a thing
# and run it.

if [ $# -eq 0 ]; then
  printf 'usage: %s <term>\n' "$0" >&2
  exit 1
fi

# Preference 1: Ripgrep
if command -v rg &>/dev/null; then
  rg --no-ignore --color always --line-number --search-zip "$@" | less -iRnSFX
  exit ${PIPESTATUS[0]}
fi


# Preference 2: The Silver Searcher
if command -v ag &>/dev/null; then
  if ag --version &>/dev/null; then
    exec ag "$@"
  fi
fi

# Preference 3: Ack
if command -v ack &>/dev/null; then
  exec ack "$@"
fi

# Fallback: grep
if command -v grep &>/dev/null; then
  find . 2>/dev/null -type f -print0 | \
    xargs -0 -P $(getconf _NPROCESSORS_ONLN) \
    grep --binary-files=without-match --color=always --extended-regex -HI "$@" | \
    grep . | less -iRnSFX
  exit ${PIPESTATUS[2]}
fi

printf 'no suitable tool found\n' >&2
exit 1
