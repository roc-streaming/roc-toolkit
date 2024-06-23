#!/usr/bin/env bash

set -eux -o pipefail

toolchain="${1:?arg1 missing: toolchain}"
compiler="${2:?arg2 missing: compiler}"
cpu="${3:?arg3 missing: QEMU CPU model}"

case "$toolchain" in
(aarch64*) qemu_cmd=qemu-aarch64 ;;
(arm*)     qemu_cmd=qemu-arm ;;
(*)
    echo >&2 "Error: unsupported arch to run tests in QEMU; toolchain: $toolchain."
    exit 1
esac
readonly qemu_cmd

find bin/"$toolchain" -name 'roc-test-*' | \
    while read tst
    do
        LD_LIBRARY_PATH="/opt/sysroot/lib:$(echo \
          "$PWD/build/3rdparty/$toolchain/$compiler"/*/rpath | tr ' ' ':')" \
            python3 scripts/scons_helpers/timeout-run.py 300 \
              "$qemu_cmd" -L "/opt/sysroot" -cpu "$cpu" "$tst"
    done
