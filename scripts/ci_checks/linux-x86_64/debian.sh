#!/usr/bin/env bash

set -euxo pipefail

scons -Q \
      --enable-werror \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
      --enable-doxygen \
      --build-3rdparty=libuv,openfec \
      test
