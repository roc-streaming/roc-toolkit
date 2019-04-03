#! /bin/bash
set -xe

brew update

brew install "scons"
brew install "gengetopt"

scons -Q clean

scons -Q \
      --enable-werror \
      --enable-debug \
      --sanitizers=address \
      --build-3rdparty=uv,openfec,sox,cpputest \
      test

scons -Q \
      --enable-werror \
      --build-3rdparty=uv,openfec,sox,cpputest \
      test

scons -Q \
      --enable-werror \
      --build-3rdparty=all \
      test
