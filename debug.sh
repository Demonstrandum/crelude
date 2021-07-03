#!/bin/sh

make clean
export TARGET=tests_debug
export CFLAGS=-ggdb
export OPT=-Og

make

printf "Run \`gdb --args %s %s\`? [y/N]: " "$TARGET" "$*"
read -r REPLY
RUN_GDB="$(echo "$REPLY" | awk '{print tolower($0)}')"
[ -z "$RUN_GDB" ] && RUN_GDB="n"
[ "$RUN_GDB" = y ] && RUN_GDB=true || RUN_GDB=false

$RUN_GDB && gdb --args "$TARGET" "$@"
