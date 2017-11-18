#!/bin/bash

# This script symlinks all relevant scripts from this repository into ~/bin and
# configuration settings into your home directory.

# Make a failure terminate the script
set -e

REPO=`dirname $0`
REPO=`readlink -f "${REPO}/.."`

mkdir -p "${HOME}/bin"

# Link useful scripts.
for i in \
         addcert.sh \
         after \
         ag \
         bashd.sh \
         check_tmux_version.sh \
         cmake \
         compresspdf.sh \
         fifi.sh \
         find-broken.sh \
         fwdmail.py \
         generate-passwd \
         g \
         ifind \
         kage \
         rsync \
         timestamp \
         toggle-screensaver \
         term \
         mediawatch.py \
         meta_c \
         meta_h \
         notate.py \
         package.py \
         pdfcrop.sh \
         prefix \
         pyman \
         search \
         sendmail.py \
         ssh-proxy \
         t \
         ubiquity \
         validate.py \
         wim ; do
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
SHELL_PATH=$(${SHELL} -c 'echo ${PATH}')
if [[ ":${SHELL_PATH}:" != *":${HOME}/bin:"* ]]; then
    echo "Warning: ${HOME}/bin, where symlinks are created, is not in your bash \$PATH." >&2
fi

# Link configurations.
for i in .agignore \
         .dircolors \
         .emacs \
         .fonts.conf \
         .gdbinit \
         .gitconfig \
         .hgrc \
         .mplayer \
         .reroute-config.py \
         .screenrc \
         .tmux.conf \
         .tmux-legacy.conf \
         .tmux-modern.conf \
         .vimrc \
         .vim \
         .wgetrc \
         .Xdefaults \
         .zsh \
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

# Link SSH config.
if [ ! -e "${HOME}/.ssh/config_mattutils" ]; then
    ln -s "${REPO}/config/.ssh/config" "${HOME}/.ssh/config_mattutils"
elif [ ! -L "${HOME}/.ssh/config_mattutils" ]; then
    echo "Warning: Skipping SSH config_mattutils that already exists." >&2
fi

# Build some utils.
if [ ! -e "${HOME}/bin/balloon" ]; then
    cc -O3 -std=c11 -W -Wall -Wextra -Werror -Wwrite-strings -Wshadow -Wmissing-declarations -o "${HOME}/bin/balloon" "${REPO}/balloon.c" $(pkg-config --libs ncurses)
fi
if [ ! -e "${HOME}/bin/rerebase" ]; then
    cc -O3 -std=c11 -W -Wall -Wextra -Werror -Wwrite-strings -Wshadow -Wmissing-declarations -o "${HOME}/bin/rerebase" "${REPO}/rerebase.c"
fi
if [ ! -e "${HOME}/bin/dif" ]; then
    c++ -O3 -std=c++11 -W -Wall -Wextra -Werror -Wwrite-strings -Wshadow -Wmissing-declarations -o "${HOME}/bin/dif" "${REPO}/dif.cpp"
fi

# Re-route links.
for i in l o u x z; do
    if [ ! -e "${HOME}/bin/meta_$i" ]; then
        ln -s "${REPO}/reroute.py" "${HOME}/bin/meta_$i"
    fi
done

if [ ! -e "${HOME}/.tmux/plugins/tpm" ]; then
    echo "Warning: Tmux Plugin Manager not found (~/.tmux/plugins/tpm)" >&2
fi
