#! /bin/bash
set -euxo pipefail

scons -Q clean

scons -Q \
    --enable-werror \
    --disable-tools \
    --enable-tests \
    --enable-benchmarks \
    --build-3rdparty=libuv,openfec,speexdsp,cpputest,google-benchmark \
    --compiler=clang \
    --host=i686-linux-android28
