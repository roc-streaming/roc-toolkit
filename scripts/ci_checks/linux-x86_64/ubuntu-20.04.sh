#!/usr/bin/env bash

set -euxo pipefail

for comp in gcc-9 clang-10
do
    scons -Q \
          --enable-werror \
          --enable-tests \
          --enable-benchmarks \
          --enable-examples \
          --build-3rdparty=openfec,cpputest \
          --compiler=${comp} \
          test
done
