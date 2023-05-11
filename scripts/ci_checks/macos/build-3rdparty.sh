#!/usr/bin/env bash

set -euxo pipefail

brew install automake scons

scons -Q \
      --enable-werror \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
      --enable-static \
      --build-3rdparty=all \
      test
