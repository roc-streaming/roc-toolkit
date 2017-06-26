#! /bin/bash
set -xe

for c in gcc-6 clang-3.9
do
  scons -Q clean
  scons -Q --enable-werror --enable-sanitizers \
    --with-3rdparty=openfec,cpputest compiler=$c variant=debug test
done

for c in gcc-6 clang-3.8 clang-3.9
do
 for v in debug release
  do
    scons -Q clean
    scons -Q --enable-werror --with-3rdparty=openfec \
        compiler=$c variant=$v test
  done
done
