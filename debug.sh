#!/bin/sh

export TARGET=tests_debug

make clean
make debug

GDB="$(which gdb || which lldb)"
ARGS="--$(which gdb && echo args)"

printf "Run \`$GDB $ARGS %s %s\`? [Y/n]: " "$TARGET" "$*"
read -r REPLY
RUN_GDB="$(echo "$REPLY" | awk '{print tolower($0)}')"
[ -z "$RUN_GDB" ] && RUN_GDB="y"
[ "$RUN_GDB" = y ] && RUN_GDB=true || RUN_GDB=false

$RUN_GDB && $GDB $ARGS "$TARGET" "$@"
