#! /bin/bash
set -euxo pipefail

tar -C "$(brew --cache)" -xvf vendor/bottles/macos-10.13.tar

brew update

brew install "scons"
brew install "ragel"
brew install "gengetopt"
brew install "libuv"
brew install "speexdsp"
brew install "sox"
brew install "google-benchmark"

scons -Q clean

scons -Q \
      --enable-werror \
      --enable-debug \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
      --sanitizers=all \
      --build-3rdparty=openfec,cpputest \
      test

scons -Q \
      --enable-werror \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
      --build-3rdparty=openfec,cpputest \
      test

scons -Q \
      --enable-werror \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
      --build-3rdparty=all \
      test
