#! /bin/bash
set -euxo pipefail

TOOLCHAIN="arm-linux-gnueabihf"
COMPILER="gcc-4.9.4-release"
CPU="cortex-a15" # armv7

scons -Q \
    --enable-werror \
    --enable-tests \
    --enable-examples \
    --build-3rdparty=libuv,libunwind,openfec,alsa,pulseaudio:10.0,speexdsp,sox,cpputest \
    --host=${TOOLCHAIN}

find bin/${TOOLCHAIN} -name 'roc-test-*' \
     -not -name 'roc-test-library' |\
    while read t
    do
        LD_LIBRARY_PATH="/opt/sysroot/lib:$(echo \
          "${PWD}"/build/3rdparty/${TOOLCHAIN}/${COMPILER}/*/rpath | tr ' ' ':')" \
            python3 scripts/scons_helpers/run-with-timeout.py 300 \
            qemu-arm -L "/opt/sysroot" -cpu ${CPU} $t
    done
