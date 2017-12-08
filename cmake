#!/usr/bin/env bash

# `readlink -f`
function readlinkf() {
  TARGET=$1
  cd "$(dirname "${TARGET}")"
  TARGET=$(basename "${TARGET}")

  while [ -L "${TARGET}" ]; do
    TARGET=$(readlink "${TARGET}")
    cd $(dirname "${TARGET}")
    TARGET=$(basename "${TARGET}")
  done

  echo "$(pwd -P)/${TARGET}"
}

ME=$(readlinkf "$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )/cmake")

for candidate in $(which -a $(basename "$0")); do
  RESOLVED=$(readlinkf "${candidate}")
  if [ "${ME}" != "${RESOLVED}" ]; then
    REAL=${RESOLVED}
  fi
done

if [ -z "${REAL}" ]; then
  echo "$(basename "$0") not found" >&2
  exit 1
fi

which ninja &>/dev/null
if [ $? -eq 0 ]; then
  exec "${REAL}" -G Ninja "$@"
else
  exec "${REAL}" "$@"
fi
