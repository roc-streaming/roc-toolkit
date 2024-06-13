#!/usr/bin/env bash

set -euxo pipefail

toolchain="arm-bcm2708hardfp-linux-gnueabi"
compiler="gcc-4.7.1-release"
cpu="arm1176" # armv6

scons -Q \
    --enable-werror \
    --enable-tests \
    --enable-examples \
    --build-3rdparty=all,pulseaudio:5.0 \
    --host=${toolchain}

find bin/${toolchain} -name 'roc-test-*' | \
    while read tst
    do
        LD_LIBRARY_PATH="/opt/sysroot/lib:$(echo \
          "${PWD}"/build/3rdparty/${toolchain}/${compiler}/*/rpath | tr ' ' ':')" \
            python3 scripts/scons_helpers/timeout-run.py 300 \
              qemu-arm -L "/opt/sysroot" -cpu ${cpu} ${tst}
    done
