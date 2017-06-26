#! /bin/bash
set -xe

scons -Q clean
scons -Q --enable-werror --with-3rdparty=openfec test

for c in gcc-4.8 gcc-4.9 gcc-5 clang-3.6 clang-3.7
do
 for v in debug release
  do
    scons -Q clean
    scons -Q --enable-werror --with-3rdparty=openfec \
        compiler=$c variant=$v test
  done
done
