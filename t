#!/usr/bin/env bash

# Rejoin a Tmux session, or start a new one if there is no current one.

export TMUX_BIN="${TMUX_BIN:-tmux}"

if ! command -v "${TMUX_BIN}" &>/dev/null; then
  printf 'tmux not found\n' >&2
  exit 1
fi

# Force Zsh if it's available. Useful in environments that don't let us change
# our default shell.
if command -v zsh &>/dev/null; then
  export SHELL=$(command -v zsh)
fi

# Sometime in the Systemd version range (251, 254] and the Tmux Fedora version
# range (3.3a-1, 3.3a-7], the two have stopped cooperating. I have not been able
# to determine the precise root cause though it may be
# https://bugzilla.redhat.com/show_bug.cgi?id=2158980 that landed as Tmux
# 3.3a-3. The symptoms are Systemd hangs on shutdown for ≥2mins waiting for
# “tmux-spawn” to exit. Detect and work around this scenario by wrapping Tmux in
# something that Systemd knows how to murder.
SYSTEMD=
if command -v systemctl &>/dev/null; then
  if command -v systemd-run &>/dev/null; then
    V=$(systemctl --version | grep '^systemd' | cut -d' ' -f 2)
    if [ ${V} -gt 251 ]; then
      SYSTEMD="systemd-run --scope --user"
    fi
  fi
fi

if "${TMUX_BIN}" list-sessions &>/dev/null; then
  exec ${SYSTEMD} ssh-agent "${TMUX_BIN}" attach
else
  exec ${SYSTEMD} ssh-agent "${TMUX_BIN}"
fi
