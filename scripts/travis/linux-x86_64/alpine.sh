#! /bin/bash
set -euxo pipefail

scons -Q clean

scons -Q \
      --enable-werror \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
      --enable-doxygen \
      --build-3rdparty=openfec,google-benchmark \
      test
