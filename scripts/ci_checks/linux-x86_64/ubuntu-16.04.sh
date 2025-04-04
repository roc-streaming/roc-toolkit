#!/usr/bin/env bash

set -euxo pipefail

for comp in gcc-4.8 clang-3.7
do
    scons -Q \
          --enable-werror \
          --enable-tests \
          --enable-examples \
          --enable-doxygen \
          --build-3rdparty=libatomic_ops,openfec,openssl,cpputest \
          --compiler=${comp} \
          test
done
