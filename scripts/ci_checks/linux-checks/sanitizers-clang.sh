#! /bin/bash
set -euxo pipefail

scons -Q \
      --enable-werror \
      --enable-debug \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
      --sanitizers=all \
      --build-3rdparty=all \
      --compiler=clang \
      test
