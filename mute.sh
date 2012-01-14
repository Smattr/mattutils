#!/bin/bash

# Symlink this in /etc/rc0.d and /etc/rc6.d because my fucking laptop only has
# soft volume keys that don't work until you've logged in.
# Update: FIXME: This actually doesn't seem to work. Hack, hack, hack...

# Mute speakers.
amixer set Master mute

