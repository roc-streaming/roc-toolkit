#! /bin/bash
set -xe

brew update

brew install "scons"
brew install "gengetopt"
brew install "libuv"

scons -Q clean

scons -Q \
      --enable-werror \
      --enable-debug \
      --enable-sanitizers \
      --build-3rdparty=sox,openfec,cpputest \
      test

scons -Q \
      --enable-werror \
      --build-3rdparty=sox,openfec,cpputest \
      test

scons -Q \
      --enable-werror \
      --build-3rdparty=all \
      test
