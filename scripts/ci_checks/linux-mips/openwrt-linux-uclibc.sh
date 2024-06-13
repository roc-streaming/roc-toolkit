#!/usr/bin/env bash

set -euxo pipefail

scons -Q \
    --enable-werror \
    --enable-tests \
    --enable-examples \
    --disable-libunwind \
    --disable-pulseaudio \
    --disable-sox \
    --disable-sndfile \
    --disable-openssl \
    --build-3rdparty=all \
    --host=mips-openwrt-linux-uclibc
