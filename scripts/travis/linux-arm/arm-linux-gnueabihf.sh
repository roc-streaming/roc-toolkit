#! /bin/bash
set -euxo pipefail

TOOLCHAIN="arm-linux-gnueabihf"
COMPILER="gcc-4.9.4-release"
CPU="cortex-a15" # armv7

scons -Q clean

scons -Q \
    --enable-werror \
    --enable-pulseaudio-modules \
    --build-3rdparty=libuv,libunwind,libatomic_ops,openfec,alsa,pulseaudio:10.0,sox,cpputest \
    --host=${TOOLCHAIN}

find bin/${TOOLCHAIN} -name 'roc-test-*' \
     -not -name 'roc-test-library' |\
    while read t
    do
        LD_LIBRARY_PATH="/opt/sysroot/lib:${PWD}/3rdparty/${TOOLCHAIN}/${COMPILER}/rpath" \
            python2 scripts/wrappers/timeout.py 300 \
            qemu-arm -L "/opt/sysroot" -cpu ${CPU} $t
    done
