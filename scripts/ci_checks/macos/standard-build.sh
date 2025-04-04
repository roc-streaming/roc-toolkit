#!/usr/bin/env bash

set -euxo pipefail

brew install --quiet --force --overwrite \
     automake scons ragel gengetopt \
     libuv speexdsp sox openssl@3 \
     cpputest libsndfile

# debug build
scons -Q \
      --enable-werror \
      --enable-tests \
      --enable-examples \
      --enable-debug \
      --sanitizers=all \
      --build-3rdparty=openfec \
      test

# release build
scons -Q \
      --enable-werror \
      --enable-tests \
      --enable-examples \
      --build-3rdparty=openfec \
      test
