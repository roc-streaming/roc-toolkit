#! /bin/bash
set -xe

TOOLCHAIN="arm-linux-gnueabihf"
CPU="cortex-a15" # armv7

scons -Q clean

scons -Q \
    --enable-werror \
    --enable-pulseaudio-modules \
    --build-3rdparty=uv,openfec,alsa,pulseaudio,sox,cpputest \
    --host=${TOOLCHAIN}

find bin/${TOOLCHAIN} -name 'roc-test-*' \
     -not -name 'roc-test-lib' |\
    while read t
    do
        LD_LIBRARY_PATH="/opt/sysroot/lib:${PWD}/3rdparty/${TOOLCHAIN}/rpath" \
            python2 site_scons/site_tools/roc/wrappers/timeout.py 300 \
            qemu-arm -L "/opt/sysroot" -cpu ${CPU} $t
    done
