#!/usr/bin/env bash

set -euxo pipefail

for comp in gcc-6 clang-6.0
do
    scons -Q \
          --enable-werror \
          --enable-tests \
          --enable-examples \
          --enable-doxygen \
          --build-3rdparty=openfec,cpputest \
          --compiler=${comp} \
          test
done
