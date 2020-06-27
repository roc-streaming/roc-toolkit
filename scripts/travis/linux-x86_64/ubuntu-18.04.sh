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
<<<<<<< HEAD
=======
          --enable-doxygen \
>>>>>>> d8f74d5d3fb22f41808e9a1d19ad46742ca33476
          --build-3rdparty=openfec,pulseaudio,google-benchmark \
          --compiler=$c \
          test
done
