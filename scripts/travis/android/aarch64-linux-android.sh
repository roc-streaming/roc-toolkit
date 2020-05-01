#! /bin/bash
set -euxo pipefail

TOOLCHAIN="aarch64-linux-android"

scons -Q clean

scons -Q \
    --enable-werror \
    --disable-tools \
    --enable-tests \
    --enable-benchmarks \
    --build-3rdparty=libuv,libatomic_ops,openfec,cpputest,google-benchmark \
    --compiler=clang \
    --host=${TOOLCHAIN}
