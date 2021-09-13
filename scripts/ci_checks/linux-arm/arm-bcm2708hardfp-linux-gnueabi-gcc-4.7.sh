#! /bin/bash
set -euxo pipefail

TOOLCHAIN="arm-bcm2708hardfp-linux-gnueabi"
COMPILER="gcc-4.7.1-release"
CPU="arm1176" # armv6

scons -Q \
    --enable-werror \
    --enable-tests \
    --enable-examples \
    --build-3rdparty=libuv,libunwind,libatomic_ops,openfec,alsa,pulseaudio:5.0,speexdsp,sox,cpputest \
    --host=${TOOLCHAIN}

find bin/${TOOLCHAIN} -name 'roc-test-*' \
     -not -name 'roc-test-library' |\
    while read t
    do
        LD_LIBRARY_PATH="/opt/sysroot/lib:$(echo \
          "${PWD}"/build/3rdparty/${TOOLCHAIN}/${COMPILER}/*/rpath | tr ' ' ':')" \
            python2 scripts/scons_helpers/run-with-timeout.py 300 \
            qemu-arm -L "/opt/sysroot" -cpu ${CPU} $t
    done
