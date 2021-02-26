#! /bin/bash
set -euxo pipefail

scons -Q clean

scons -Q \
      --enable-werror \
      --disable-soversion \
      --disable-tools \
      --enable-tests \
      --build-3rdparty=libuv,openfec,speexdsp,cpputest \
      --compiler=clang \
      --host="$1"
