#! /bin/bash
set -xe

scons -Q clean

for c in gcc-4.4 gcc-4.6 clang-3.4
do
    scons -Q \
          --enable-werror \
          --compiler=$c \
          --build-3rdparty=uv,openfec,cpputest \
          test
done
