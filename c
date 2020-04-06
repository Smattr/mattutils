#!/usr/bin/env bash

# if Clink is installed, run that
if which clink &>/dev/null; then
  exec clink "$@"
fi

# if we have cscope, use that
if which cscope &>/dev/null; then
  if [ $# -eq 0 ]; then
    exec cscope -R
  else
    exec cscope "$@"
  fi
fi

printf 'neither Clink nor Cscope found\n' >&2
exit 1
