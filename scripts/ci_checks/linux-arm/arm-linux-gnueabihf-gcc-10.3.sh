#!/usr/bin/env bash

set -eux -o pipefail

toolchain="arm-none-linux-gnueabihf"
compiler="gcc-10.3.1-release"
cpu="cortex-a15" # armv7

scons -Q \
    --enable-werror \
    --enable-tests \
    --enable-examples \
    --build-3rdparty=all,pulseaudio:8.0 \
    --host=${toolchain}

"$( dirname "$0" )"/run-tests-in-qemu.sh "$toolchain" "$compiler" "$cpu"
