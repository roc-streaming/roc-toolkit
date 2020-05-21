#! /bin/bash
set -euxo pipefail

scons -Q clean

for c in gcc-7 clang-7 gcc-8 clang-8 gcc-9 clang-9
do
    scons -Q \
          --enable-werror \
          --enable-pulseaudio-modules \
          --enable-tests \
          --enable-benchmarks \
          --enable-examples \
          --build-3rdparty=openfec,pulseaudio \
          --compiler=$c \
          test
done
