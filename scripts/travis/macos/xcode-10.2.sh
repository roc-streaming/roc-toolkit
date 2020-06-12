#! /bin/bash
set -euxo pipefail

brew update

brew install "scons"
brew install "ragel"
brew install "gengetopt"
brew install "libuv"
brew install "speexdsp"
brew install "sox"
brew install "cpputest"
brew install "google-benchmark"

scons -Q clean

scons -Q \
      --enable-werror \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
      --build-3rdparty=openfec \
      test
