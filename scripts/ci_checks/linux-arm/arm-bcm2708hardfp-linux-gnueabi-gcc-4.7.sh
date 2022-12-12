#!/usr/bin/env bash

set -euxo pipefail

toolchain="arm-bcm2708hardfp-linux-gnueabi"
compiler="gcc-4.7.1-release"
cpu="arm1176" # armv6

scons -Q \
    --enable-werror \
    --enable-tests \
    --enable-examples \
    --build-3rdparty=libuv,libunwind,libatomic_ops,openfec,alsa,pulseaudio:5.0,speexdsp,sox,cpputest \
    --host=${toolchain}

find bin/${toolchain} -name 'roc-test-*' \
     -not -name 'roc-test-library' |\
    while read t
    do
        LD_LIBRARY_PATH="/opt/sysroot/lib:$(echo \
          "${PWD}"/build/3rdparty/${toolchain}/${compiler}/*/rpath | tr ' ' ':')" \
            python3 scripts/scons_helpers/run-with-timeout.py 300 \
              qemu-arm -L "/opt/sysroot" -cpu ${cpu} $t
    done
