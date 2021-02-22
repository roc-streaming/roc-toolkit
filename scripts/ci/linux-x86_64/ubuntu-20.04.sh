#! /bin/bash
set -euxo pipefail

scons -Q clean

for c in gcc-8 clang-8 gcc-10 clang-10
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
