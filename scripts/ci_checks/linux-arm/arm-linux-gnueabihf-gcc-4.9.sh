#!/usr/bin/env bash

set -euxo pipefail

toolchain="arm-linux-gnueabihf"
compiler="gcc-4.9.4-release"
cpu="cortex-a15" # armv7

scons -Q \
    --enable-werror \
    --enable-tests \
    --enable-examples \
    --build-3rdparty=libuv,libunwind,openfec,alsa,pulseaudio:10.0,speexdsp,sox,openssl,cpputest \
    --host="${toolchain}"

find bin/${toolchain} -name 'roc-test-*' \
     -not -name 'roc-test-library' | \
    while read t
    do
        LD_LIBRARY_PATH="/opt/sysroot/lib:$(echo \
          "${PWD}"/build/3rdparty/${toolchain}/${compiler}/*/rpath | tr ' ' ':')" \
            python3 scripts/scons_helpers/run-with-timeout.py 300 \
              qemu-arm -L "/opt/sysroot" -cpu "${cpu}" "$t"
    done
