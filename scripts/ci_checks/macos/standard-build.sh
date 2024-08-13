#!/usr/bin/env bash

set -euxo pipefail

brew install --quiet --force --overwrite \
     automake scons ragel gengetopt \
     libuv speexdsp sox openssl@3 \
     cpputest google-benchmark libsndfile

# debug build
scons -Q \
      --enable-werror \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
      --enable-debug \
      --sanitizers=all \
      --build-3rdparty=openfec \
      test

# release build
scons -Q \
      --enable-werror \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
      --build-3rdparty=openfec \
      test
