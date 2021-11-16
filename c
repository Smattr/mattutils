#!/usr/bin/env bash

# if Clink is installed, run that
if command -v clink &>/dev/null; then

  # on macOS, assume Clink may be dynamically linked against a
  # Macports-installed libclang that it will need some help locating
  if [ "$(uname -s)" = "Darwin" ]; then
    for v in 10 9 8 7 6; do
      if [ -e "/opt/local/libexec/llvm-${v}.0" ]; then
        export LD_LIBRARY_PATH=${LD_LIRBRARY_PATH:-${LD_LIBRARY_PATH}:}/opt/local/libexec/llvm-${v}.0/lib
      fi
    done
  fi

  exec clink "$@"
fi

# if we have cscope, use that
if command -v cscope &>/dev/null; then
  if [ $# -eq 0 ]; then
    exec cscope -R
  else
    exec cscope "$@"
  fi
fi

printf 'neither Clink nor Cscope found\n' >&2
exit 1
