#! /bin/bash
set -xe

scons -Q clean

scons -Q \
      --enable-werror \
      --enable-debug \
      --enable-sanitizers \
      --build-3rdparty=openfec,cpputest \
      test

scons -Q \
      --enable-werror \
      --build-3rdparty=openfec,cpputest \
      test

scons -Q \
      --enable-werror \
      --build-3rdparty=all \
      test
