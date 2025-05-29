ifdef VERBOSE
  V :=
else
  V := @
endif

CC ?= cc
CFLAGS ?= -std=c11 -O3 -W -Wall -Wextra -Wshadow -Wwrite-strings -Wmissing-prototypes

# Serenity now
SHELL := $(shell which bash 2>/dev/null)

default: \
  ${HOME}/.ssh/config_mattutils \
  ${HOME}/.agignore \
  ${HOME}/.dircolors \
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
  ${HOME}/.vimrc \
  ${HOME}/.vim \
  ${HOME}/.wgetrc \
  ${HOME}/.XCompose \
  ${HOME}/.Xdefaults \
  ${HOME}/.zshrc \
  ${HOME}/bin/ag \
  ${HOME}/bin/bashd.sh \
  ${HOME}/bin/c \
  ${HOME}/bin/check_tmux_version.sh \
  ${HOME}/bin/cub.py \
  ${HOME}/bin/dif \
  ${HOME}/bin/find-broken.sh \
  ${HOME}/bin/fwdmail.py \
  ${HOME}/bin/generate-passwd \
  ${HOME}/bin/g \
  ${HOME}/bin/git-cleanup.py \
  ${HOME}/bin/git-edit.py \
  ${HOME}/bin/git-increment.py \
  ${HOME}/bin/git-retime.py \
  ${HOME}/bin/git-reup.py \
  ${HOME}/bin/ifind \
  ${HOME}/bin/loc \
  ${HOME}/bin/meta_u \
  ${HOME}/bin/oi \
  ${HOME}/bin/prefix \
  ${HOME}/bin/pyman \
  ${HOME}/bin/rerebase \
  ${HOME}/bin/shrink \
  ${HOME}/bin/s \
  ${HOME}/bin/t \
  ${HOME}/bin/term \
  ${HOME}/bin/timestamp \
  ${HOME}/bin/update.py \
  ${HOME}/bin/v \
  ${HOME}/bin/validate.py \
  ${HOME}/bin/wim \
  ${HOME}/bin/z \


${HOME}/.ssh/config_mattutils: config/.ssh/config Makefile
	@printf ' [LN] %s\n' "$(notdir $@)"
	${V}if [ ! -e "$@" ]; then mkdir -p ${HOME}/.ssh && ln -s $$(pwd)/$< $@; elif [ ! -L "$@" ]; then printf 'Warning: Skipping %s that already exists\n' "$@" >&2; fi


${HOME}/bin/dif: dif.c
	@printf ' [CC] %s\n' "$(notdir $@)"
	${V}mkdir -p "$(dir $@)"
	${V}${CC} ${CFLAGS} -o $@ $<

${HOME}/bin/oi: oi/oi Makefile
	@printf ' [LN] oi\n'
	${V}mkdir -p "$(dir $@)"
	${V}if [ ! -e "$@" ]; then ln -s $$(pwd)/$< $@; elif [ ! -L "$@" ]; then printf 'Warning: Skipping %s that already exists\n' "$@" >&2; fi

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
