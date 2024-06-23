#!/usr/bin/env bash

set -eux -o pipefail

toolchain="arm-bcm2708hardfp-linux-gnueabi"
compiler="gcc-4.7.1-release"
cpu="arm1176" # armv6

scons -Q \
    --enable-werror \
    --enable-tests \
    --enable-examples \
    --build-3rdparty=all,pulseaudio:5.0 \
    --host=${toolchain}

"$( dirname "$0" )"/run-tests-in-qemu.sh "$toolchain" "$compiler" "$cpu"
