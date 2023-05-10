#!/usr/bin/env bash

set -euxo pipefail

for comp in gcc-11 gcc-12 clang-11 clang-14
do
    scons -Q \
          --enable-werror \
          --enable-tests \
          --enable-benchmarks \
          --enable-examples \
          --build-3rdparty=openfec \
          --compiler=${comp} \
          test
done
