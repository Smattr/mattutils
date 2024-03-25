#!/usr/bin/env bash

# shortcut for Vim

# support path:lineno syntax
argv0=
argv1=
if [ $# -gt 0 ]; then
  IFS=':' read path lineno <<< "$1"
  if [ -n "${lineno}" ]; then
    argv0="${path}"
    argv1="+0${lineno}"
    shift
  fi
fi

if which vim &>/dev/null; then
  exec vim ${argv0} ${argv1} "$@"
fi

vi ${argv0} ${argv1} "$@"
