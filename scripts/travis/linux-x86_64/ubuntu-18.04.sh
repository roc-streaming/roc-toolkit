#! /bin/bash
set -euxo pipefail

scons -Q clean

for c in gcc-6 clang-6.0
do
    scons -Q \
          --enable-werror \
          --enable-pulseaudio-modules \
          --enable-tests \
          --enable-benchmarks \
          --enable-examples \
          --enable-doxygen \
          --build-3rdparty=openfec,pulseaudio,google-benchmark \
          --compiler=$c \
          test
done
