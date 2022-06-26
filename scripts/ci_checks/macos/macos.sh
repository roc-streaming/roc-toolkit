#! /bin/bash
set -euxo pipefail

brew install \
     automake scons ragel gengetopt\
     libuv speexdsp sox \
     cpputest google-benchmark

scons -Q \
      --enable-werror \
      --enable-debug \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
      --sanitizers=all \
      --build-3rdparty=openfec \
      test

scons -Q \
      --enable-werror \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
      --build-3rdparty=openfec \
      test

scons -Q \
      --enable-werror \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
      --build-3rdparty=all \
      test
