#!/usr/bin/env bash

set -euxo pipefail

for comp in gcc-15 clang-21
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
