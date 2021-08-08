#!/usr/bin/env bash

if which vim &>/dev/null; then
  exec vim "$@"
fi

vi "$@"
