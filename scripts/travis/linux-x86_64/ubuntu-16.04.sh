#! /bin/bash
set -euxo pipefail

scons -Q clean

scons -Q \
      --enable-werror \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
      --build-3rdparty=openfec,google-benchmark \
      test

for c in gcc-4.8 gcc-5 clang-3.7
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
          --build-3rdparty=libatomic_ops,openfec,pulseaudio,google-benchmark \
          --compiler=$c \
          test
done
