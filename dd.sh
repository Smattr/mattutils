#!/bin/bash

# Wrapper for dd to report progress as it's copying.

dd "$@" &
PID=$!

trap "kill ${PID} ; exit" SIGINT SIGTERM SIGQUIT

# Wait for dd to start up. Without this there's a race and we can signal dd too
# early (before it's registered its signal handler) and cause it to exit.
sleep 1

# While dd's still running, send it SIGUSR1 that triggers a progress print.
while [ `kill -0 ${PID} &>/dev/null ; echo $?` -eq 0 ]; do
    kill -SIGUSR1 ${PID}
    sleep 2
done

# Clean up.
wait ${PID}
