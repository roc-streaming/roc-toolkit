#! /bin/bash
set -xe

scons -Q clean

scons -Q \
      --enable-werror \
      --enable-debug \
      --sanitizers=all \
      --build-3rdparty=openfec,cpputest \
      --compiler=clang-3.9 \
      test

for c in gcc-6 clang-3.9
do
    scons -Q \
          --enable-werror \
          --enable-pulseaudio-modules \
          --build-3rdparty=openfec,pulseaudio \
          --compiler=$c \
          test
done
