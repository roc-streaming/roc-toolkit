#! /bin/bash
set -xe

scons -Q clean

for c in clang-3.9
do
    scons -Q \
          --enable-werror \
          --enable-debug \
          --enable-sanitizers \
          --compiler=$c \
          --build-3rdparty=openfec,cpputest \
          test
done

for c in gcc-6 clang-3.9
do
    scons -Q \
          --enable-werror \
          --compiler=$c \
          --build-3rdparty=openfec \
          test
done
