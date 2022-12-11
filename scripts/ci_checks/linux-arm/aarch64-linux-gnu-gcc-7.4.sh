#!/usr/bin/env bash

set -euxo pipefail

toolchain="aarch64-linux-gnu"
compiler="gcc-7.4.1-release"
cpu="cortex-a53" # armv8

for pulse in 8.0 15.99.1
do
    scons -Q \
        --enable-werror \
        --enable-tests \
        --enable-examples \
        --build-3rdparty=libuv,libunwind,openfec,alsa,pulseaudio:$pulse,speexdsp,sox,openssl,cpputest \
        --host="${toolchain}"

    find bin/${toolchain} -name 'roc-test-*' \
         -not -name 'roc-test-library' | \
        while read t
        do
            LD_LIBRARY_PATH="/opt/sysroot/lib:$(echo \
              "${PWD}"/build/3rdparty/${toolchain}/${compiler}/*/rpath | tr ' ' ':')" \
                python3 scripts/scons_helpers/run-with-timeout.py 300 \
                  qemu-aarch64 -L "/opt/sysroot" -cpu "${cpu}" "$t"
        done
done
