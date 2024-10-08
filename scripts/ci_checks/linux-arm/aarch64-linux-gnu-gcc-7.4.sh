#!/usr/bin/env bash

set -eux -o pipefail

toolchain="aarch64-linux-gnu"
compiler="gcc-7.4.1-release"
cpu="cortex-a53" # armv8

scons -Q \
    --enable-werror \
    --enable-tests \
    --enable-examples \
    --build-3rdparty=all,pulseaudio:10.0 \
    --host=${toolchain}

"$( dirname "$0" )"/run-tests-in-qemu.sh "$toolchain" "$compiler" "$cpu"
