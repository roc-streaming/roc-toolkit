#!/usr/bin/env bash

set -euxo pipefail

toolchain="arm-linux-gnueabihf"
compiler="gcc-4.9.4-release"
cpu="cortex-a15" # armv7

third_party="libuv,libunwind,openfec,alsa,pulseaudio:10.0,speexdsp,sox,openssl,cpputest,sndfile"

scons -Q \
    --enable-werror \
    --enable-tests \
    --enable-examples \
    --build-3rdparty=${third_party} \
    --host=${toolchain}

find bin/${toolchain} -name 'roc-test-*' | \
    while read tst
    do
        LD_LIBRARY_PATH="/opt/sysroot/lib:$(echo \
          "${PWD}"/build/3rdparty/${toolchain}/${compiler}/*/rpath | tr ' ' ':')" \
            python3 scripts/scons_helpers/timeout-run.py 300 \
              qemu-arm -L "/opt/sysroot" -cpu ${cpu} ${tst}
    done
