#!/usr/bin/env bash

set -euxo pipefail

for comp in gcc-13 clang-14 clang-16 clang-19
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
