#!/usr/bin/env bash

set -euxo pipefail

# gcc
scons -Q \
      --enable-werror \
      --enable-debug \
      --enable-tests \
      --enable-examples \
      --sanitizers=all \
      --build-3rdparty=all \
      --compiler=gcc \
      test

# clang
scons -Q \
      --enable-werror \
      --enable-debug \
      --enable-tests \
      --enable-examples \
      --sanitizers=all \
      --build-3rdparty=all \
      --compiler=clang \
      test
