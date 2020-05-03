#! /bin/bash
set -euxo pipefail

scons -Q clean

scons -Q \
      --enable-werror \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
      --build-3rdparty=openfec,google-benchmark:1.2.0 \
      test

for c in gcc-4.8 gcc-5 clang-3.7
do
    scons -Q \
          --enable-werror \
          --enable-pulseaudio-modules \
          --enable-tests \
          --enable-benchmarks \
          --enable-examples \
          --build-3rdparty=openfec,pulseaudio,google-benchmark:1.2.0 \
          --compiler=$c \
          test
done
