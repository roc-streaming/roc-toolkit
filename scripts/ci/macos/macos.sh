#! /bin/bash
set -euxo pipefail

brew install "automake"
brew install "scons"
brew install "ragel"
brew install "gengetopt"
brew install "libuv"
brew install "sox"
brew install "cpputest"

scons -Q clean

scons -Q \
      --enable-werror \
      --enable-debug \
      --sanitizers=all \
      --build-3rdparty=openfec \
      test

scons -Q \
      --enable-werror \
      --build-3rdparty=openfec \
      test

scons -Q \
      --enable-werror \
      --build-3rdparty=all \
      test
