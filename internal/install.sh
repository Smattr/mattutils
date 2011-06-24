#!/bin/bash

# This script symlinks all relevant scripts from this repository into ~/bin and
# configuration settings into your home directory.

# Make a failure terminate the script
set -e

REPO=`dirname $0`
REPO=`readlink -f "${REPO}/.."`

# Check that we're on the private branch. This repo has two branches, master
# and private. Private contains everything in master plus some extra bits that
# are confidential. The master branch is hosted on github and my private
# server, while the private branch is only hosted on my private server. If
# you're installing from master it's most likely you forgot to switch branches
# and are going to miss some scripts/configs.
if [ -z "`which git`" ]; then
    echo "Error: Git is not installed or not in your \$PATH." >&2
    exit 1
fi
pushd "${REPO}" >/dev/null
if [ "`git branch --color=never | grep --color=never "^\*." | cut -d" " -f 2`" == "master" ]; then
    echo "Error: Your working directory is on the master branch." >&2
    exit 1
fi
popd >/dev/null

mkdir -p "${HOME}/bin"

# Link useful scripts.
for i in generate-passwd \
         gg \
         github-ls.sh \
         has-changed.sh \
         have-lib.sh \
         playtime \
         toggle-screensaver; do
    if [ ! -e "${REPO}/$i" ]; then
        echo "Error: $i not found in working directory." >&2
        exit 1
    fi
    if [ ! -e "${HOME}/bin/$i" ]; then
        ln -s "${REPO}/$i" "${HOME}/bin/$i"
    elif [ ! -L "${HOME}/bin/$i" ]; then
        echo "Warning: Skipping $i that already exists." >&2
    fi
done
if [ ":${PATH}:" != *":${HOME}/bin:"* ]; then
    echo "Warning: ${HOME}/bin, where symlinks are created, is not in your bash \$PATH." >&2
fi

# Link configurations.
for i in .dircolors \
         .gdbinit \
         .hgrc \
         .screenrc \
         .vimrc \
         .zshrc; do
    if [ ! -e "${REPO}/config/$i" ]; then
        echo "Error: $i not found in working directory." >&2
        exit 1
    fi
    if [ ! -e "${HOME}/$i" ]; then
        ln -s "${REPO}/config/$i" "${HOME}/$i"
    elif [ ! -L "${HOME}/$i" ]; then
        echo "Warning: Skipping $i that already exists." >&2
    fi
done

# Link HTTPS Everywhere rules.
if [ `find "${HOME}/.mozilla" -type d -name HTTPSEverywhereUserRules | wc -l` -ne 1 ]; then
    echo "Error: could not determine your HTTPS Everwhere rules directory." >&2
else
    HTTPS_RULES=`find "${HOME}/.mozilla" -type d -name HTTPSEverywhereUserRules`
    for i in `find "${REPO}/config/HTTPSEverywhereUserRules" -type f -iname "*.xml" -exec echo "{}" \;`; do
        DEST=${HTTPS_RULES}/`basename "$i"`
        if [ ! -e "${DEST}" ]; then
            ln -s "$i" "${DEST}"
        else
            echo "Warning: Skipping HTTPS Everywhere rule "`basename "$i"`" that already exists." >&2
        fi
    done
fi

# SSH
if [ ! -e "${HOME}/.ssh" ]; then
    echo "Warning: Skipping all SSH setup because ~/.ssh doesn't exist." >&2
else
    if [ ! -e "${HOME}/.ssh/config" ]; then
        ln -s "${REPO}/config/.ssh/config" "${HOME}/.ssh/config"
    elif [ ! -L "${HOME}/.ssh/config" ]; then
        echo "Warning: Skipping ~/.ssh/config that already exists." >&2
    fi
fi
