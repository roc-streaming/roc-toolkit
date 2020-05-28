#! /bin/bash
set -euxo pipefail

scons -Q clean

scons -Q --enable-werror --build-3rdparty=all \
      --enable-debug \
      --enable-debug-3rdparty \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
      test
