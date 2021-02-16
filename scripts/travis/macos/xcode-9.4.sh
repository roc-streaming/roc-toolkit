#! /bin/bash
set -euxo pipefail

brew cask uninstall --force java

brew update || true

brew unlink python@2
brew list |\
    grep -vE 'pkg-config|automake|libtool|cmake|xz|readline|openssl|sqlite|python|gdbm|xquartz' |\
    xargs brew pin

brew install --build-from-source "scons"
brew install "ragel"
brew install "gengetopt"
brew install "libuv"
brew install "speexdsp"
brew install "sox"
brew install "google-benchmark"

scons -Q clean

scons -Q \
      --enable-werror \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
      --build-3rdparty=openfec,cpputest \
      test
