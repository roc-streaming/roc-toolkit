#!/usr/bin/env bash

set -euxo pipefail

brew install \
     automake scons ragel gengetopt \
     libuv speexdsp sox openssl@3 \
     cpputest google-benchmark

# > openssl@3 is keg-only, which means it was not symlinked into /usr/local,
# > because macOS provides LibreSSL.
export PKG_CONFIG_PATH="/usr/local/opt/openssl@3/lib/pkgconfig:${PKG_CONFIG_PATH:-}"

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
