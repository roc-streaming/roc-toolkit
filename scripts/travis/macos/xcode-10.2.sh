#! /bin/bash
set -euxo pipefail

brew update

brew install "scons"
brew install "ragel"
brew install "gengetopt"
brew install "libuv"
brew install "libatomic_ops"
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
