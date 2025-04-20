#!/usr/bin/env bash

set -euxo pipefail

for comp in gcc-4.8 clang-3.4
do
    scons -Q \
          --enable-werror \
          --enable-tests \
          --enable-examples \
          --enable-doxygen \
          --build-3rdparty=libuv,libatomic_ops,openfec,openssl:1.1.1t,pulseaudio,cpputest \
          --compiler=${comp} \
          test
done
