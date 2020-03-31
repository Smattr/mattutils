#!/usr/bin/env bash

# Grep is indispensible for navigating large, dynamic code bases, but you often
# want a smarter, faster alternative. This script tries to find you such a thing
# and run it.

if [ $# -eq 0 ]; then
  printf 'usage: %s <term>\n' "$0" >&2
  exit 1
fi

# Preference 1: Ripgrep
if which rg &>/dev/null; then
  rg --no-ignore --color always --line-number --search-zip "$@" | less -iRnSFX
  exit ${PIPESTATUS[0]}
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

# Fallback: egrep or grep
for GREP in egrep grep; do
  if which ${GREP} &>/dev/null; then
    find . 2>/dev/null -type f -print0 | xargs -0 -P $(getconf _NPROCESSORS_ONLN) ${GREP} --color=always -HI "$@" | less -iRnSFX
    exit 0
  fi
done

printf 'no suitable tool found\n' >&2
exit 1
