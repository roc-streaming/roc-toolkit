#!/usr/bin/env bash

set -euxo pipefail

scons -Q \
      --enable-werror \
      --disable-soversion \
      --disable-tools \
      --enable-tests \
      --build-3rdparty=libuv,openfec,speexdsp,openssl,cpputest \
      --compiler=clang \
      --host="$1"
