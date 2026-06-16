#!/bin/bash
# Tick 2.0 test suite.
# Each test program returns 0 on success. We verify, for every test:
#   1. it compiles and runs in default (checked) mode
#   2. it compiles and runs in --release (unchecked) mode
#   3. it is memory-clean under AddressSanitizer + UBSan
set -u

TICK="./build/tick"
RUNTIME="src/runtime"
DIR="tests/suite"
TMP="/tmp/tick_suite"
mkdir -p "$TMP"

pass=0
fail=0

for src in "$DIR"/*.tick; do
    name=$(basename "$src" .tick)
    ok=1

    # default (checked) build
    if ! "$TICK" "$src" -o "$TMP/$name" >/dev/null 2>"$TMP/$name.err"; then
        echo "FAIL $name: default compile"; cat "$TMP/$name.err"; ok=0
    elif ! "$TMP/$name" >/dev/null 2>&1; then
        echo "FAIL $name: default run (exit $?)"; ok=0
    fi

    # release build
    if ! "$TICK" "$src" --release -o "$TMP/${name}_r" >/dev/null 2>&1; then
        echo "FAIL $name: release compile"; ok=0
    elif ! "$TMP/${name}_r" >/dev/null 2>&1; then
        echo "FAIL $name: release run"; ok=0
    fi

    # sanitizer build (memory soundness)
    "$TICK" "$src" --keep-c -o "$TMP/${name}_s" >/dev/null 2>&1
    if cc -fsanitize=address,undefined -I"$RUNTIME" "$TMP/${name}_s.c" \
          "$RUNTIME/tick_runtime.c" -o "$TMP/${name}_asan" -pthread -lm >/dev/null 2>&1; then
        if ! "$TMP/${name}_asan" >/dev/null 2>"$TMP/$name.asan"; then
            echo "FAIL $name: sanitizer"; cat "$TMP/$name.asan"; ok=0
        fi
    fi
    rm -f "$TMP/${name}_s.c"

    if [ $ok -eq 1 ]; then echo "PASS $name"; pass=$((pass+1)); else fail=$((fail+1)); fi
done

echo "----------------------------------------"
echo "Passed: $pass   Failed: $fail"
[ $fail -eq 0 ]
