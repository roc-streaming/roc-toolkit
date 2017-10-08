#! /bin/bash
set -xe

scons -Q clean
scons -Q --enable-werror --build-3rdparty=openfec test

for c in gcc-4.8 gcc-5 clang-3.7
do
    scons -Q \
          --enable-werror \
          --compiler=$c \
          --build-3rdparty=openfec \
          test
done
