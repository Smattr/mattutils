ifdef VERBOSE
  V :=
else
  V := @
endif

CC ?= cc
CFLAGS ?= -std=c11 -O3 -W -Wall -Wextra -Wshadow -Wwrite-strings

CXX ?= c++
CXXFLAGS ?= -std=c++11 -O3 -W -Wall -Wextra -Wshadow -Wwrite-strings -Wmissing-declarations

# Serenity now
SHELL := $(shell which bash)

default: \
  ${HOME}/.ssh/config_mattutils \
  ${HOME}/.agignore \
  ${HOME}/.dircolors \
  ${HOME}/.emacs \
  ${HOME}/.fonts.conf \
  ${HOME}/.gdbinit \
  ${HOME}/.gitconfig \
  ${HOME}/.hgrc \
  ${HOME}/.mplayer \
  ${HOME}/.screenrc \
  ${HOME}/.tmux.conf \
  ${HOME}/.tmux-new-pane_current_path.conf \
  ${HOME}/.tmux-new-vi-copy.conf \
  ${HOME}/.tmux-old-pane_current_path.conf \
  ${HOME}/.tmux-old-vi-copy.conf \
  ${HOME}/.tmux-tpm.conf \
  ${HOME}/.typist.json \
  ${HOME}/.vimrc \
  ${HOME}/.vim \
  ${HOME}/.wgetrc \
  ${HOME}/.Xdefaults \
  ${HOME}/.zshrc \
  ${HOME}/bin/addcert.sh \
  ${HOME}/bin/after \
  ${HOME}/bin/ag \
  ${HOME}/bin/bashd.sh \
  ${HOME}/bin/c \
  ${HOME}/bin/check_tmux_version.sh \
  ${HOME}/bin/compress.py \
  ${HOME}/bin/cub \
  ${HOME}/bin/dif \
  ${HOME}/bin/fifi.sh \
  ${HOME}/bin/find-broken.sh \
  ${HOME}/bin/fwdmail.py \
  ${HOME}/bin/generate-passwd \
  ${HOME}/bin/g \
  ${HOME}/bin/hide \
  ${HOME}/bin/ifind \
  ${HOME}/bin/loc \
  ${HOME}/bin/meta_u \
  ${HOME}/bin/no \
  ${HOME}/bin/notate.py \
  ${HOME}/bin/prefix \
  ${HOME}/bin/pye \
  ${HOME}/bin/pyman \
  ${HOME}/bin/rerebase \
  ${HOME}/bin/search \
  ${HOME}/bin/shrink \
  ${HOME}/bin/ssh-proxy \
  ${HOME}/bin/t \
  ${HOME}/bin/term \
  ${HOME}/bin/timestamp \
  ${HOME}/bin/ubiquity \
  ${HOME}/bin/validate.py \
  ${HOME}/bin/wim \


${HOME}/.ssh/config_mattutils: config/.ssh/config Makefile
	@printf ' [LN] %s\n' "$(notdir $@)"
	${V}if [ ! -e "$@" ]; then ln -s $$(pwd)/$< $@; elif [ ! -L "$@" ]; then printf 'Warning: Skipping %s that already exists\n' "$@" >&2; fi


${HOME}/bin/cub: cub.c Makefile
	@printf ' [CC] %s\n' "$(notdir $@)"
	${V}mkdir -p "$(dir $@)"
	${V}${CC} ${CFLAGS} -o $@ $<

${HOME}/bin/dif: dif.cpp
	@printf ' [CXX] %s\n' "$(notdir $@)"
	${V}mkdir -p "$(dir $@)"
	${V}${CXX} ${CXXFLAGS} -o $@ $<

${HOME}/.%: config/.% Makefile
	@printf ' [LN] %s\n' "$(notdir $@)"
	${V}if [ ! -e "$@" ]; then ln -s $$(pwd)/$< $@; elif [ ! -L "$@" ]; then printf 'Warning: Skipping %s that already exists\n' "$@" >&2; fi

${HOME}/bin/%: % Makefile
	@printf ' [LN] %s\n' "$(notdir $@)"
	${V}mkdir -p "$(dir $@)"
	${V}if [ ! -e "$@" ]; then ln -s $$(pwd)/$< $@; elif [ ! -L "$@" ]; then printf 'Warning: Skipping %s that already exists\n' "$@" >&2; fi

${HOME}/bin/rerebase: rerebase.c Makefile
	@printf ' [CC] %s\n' "$(notdir $@)"
	${V}mkdir -p "$(dir $@)"
	${V}${CC} ${CFLAGS} -o $@ $<

${HOME}/.tmux/plugins/tpm: Makefile
	@printf 'Warning: Tmux Plugin Manager not found (~/.tmux/plugins/tpm)\n' >&2

# Disable built-in rules.
.SUFFIXES:
