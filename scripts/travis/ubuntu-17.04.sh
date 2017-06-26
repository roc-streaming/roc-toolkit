#! /bin/bash
set -xe

for c in gcc-6 clang-3.9
do
  scons -Q clean
  scons -Q --enable-werror --enable-sanitizers --with-3rdparty=openfec,cpputest \
        compiler=$c variant=debug test
done

for c in gcc-6 clang-3.9
do
  scons -Q clean
  scons -Q --enable-werror --with-3rdparty=openfec \
        compiler=$c test
done
