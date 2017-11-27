#!/usr/bin/env bash

# From certain environments like a crontab, notify-send does not have access to
# the DBUS_SESSION_BUS_ADDRESS variable that it needs to determine where to
# display notifications. This wrapper attempts to locate your Gnome session and
# determine its DBUS_SESSION_BUS_ADDRESS automatically.

# Adapted from http://stackoverflow.com/questions/23415117/shell-script-with-export-command-and-notify-send-via-crontab-not-working-export

if [ -z "${DBUS_SESSION_BUS_ADDRESS}" ]; then

  GNOME_PID=$(pgrep gnome-session)
  if [ -z "${GNOME_PID}" ]; then
    echo "Gnome session not found" >&2
    exit 1
  fi

  DBUS_SESSION_BUS_ADDRESS=$(grep -z DBUS_SESSION_BUS_ADDRESS /proc/${GNOME_PID}/environ | cut -d= -f2-)
  if [ -z "${DBUS_SESSION_BUS_ADDRESS}" ]; then
    echo "Could not locate DBUS_SESSION_BUS_ADDRESS" >&2
    exit 1
  fi

  export DBUS_SESSION_BUS_ADDRESS

fi

notify-send "$@"
