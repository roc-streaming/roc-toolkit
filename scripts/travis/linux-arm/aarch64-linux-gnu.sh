#! /bin/bash
set -euxo pipefail

TOOLCHAIN="aarch64-linux-gnu"
COMPILER="gcc-7.4.1-release"
CPU="cortex-a53" # armv8

scons -Q clean

scons -Q \
    --enable-werror \
    --enable-pulseaudio-modules \
    --enable-tests \
    --enable-examples \
    --build-3rdparty=libuv,libunwind,openfec,alsa,pulseaudio:8.0,speexdsp,sox,cpputest \
    --host=${TOOLCHAIN}

find bin/${TOOLCHAIN} -name 'roc-test-*' \
     -not -name 'roc-test-library' |\
    while read t
    do
        LD_LIBRARY_PATH="/opt/sysroot/lib:${PWD}/3rdparty/${TOOLCHAIN}/${COMPILER}/rpath" \
            python2 scripts/wrappers/timeout.py 300 \
            qemu-aarch64 -L "/opt/sysroot" -cpu ${CPU} $t
    done
