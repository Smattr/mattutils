# TODO: subsume internal/install.sh

CC ?= cc
CFLAGS ?= -std=c11 -W -Wall -Wextra -Wshadow -Wwrite-strings

# Serenity now
SHELL := $(shell which bash)

default: ${HOME}/bin/cub

${HOME}/bin/cub: cub.c Makefile
	${CC} ${CFLAGS} -o $@ $<
