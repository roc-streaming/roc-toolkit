#!/usr/bin/env bash

set -euxo pipefail

toolchain="aarch64-linux-gnu"
compiler="gcc-7.4.1-release"
cpu="cortex-a53" # armv8

scons -Q \
    --enable-werror \
    --enable-tests \
    --enable-examples \
    --build-3rdparty=all,pulseaudio:10.0 \
    --host=${toolchain}

find bin/${toolchain} -name 'roc-test-*' | \
    while read tst
    do
        LD_LIBRARY_PATH="/opt/sysroot/lib:$(echo \
          "${PWD}"/build/3rdparty/${toolchain}/${compiler}/*/rpath | tr ' ' ':')" \
            python3 scripts/scons_helpers/timeout-run.py 300 \
              qemu-aarch64 -L "/opt/sysroot" -cpu ${cpu} ${tst}
    done
