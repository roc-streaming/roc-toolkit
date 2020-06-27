#! /bin/bash
set -euxo pipefail

scons -Q clean

for c in gcc-4.4 gcc-4.6 clang-3.4
do
    scons -Q \
          --enable-werror \
          --enable-pulseaudio-modules \
          --enable-tests \
          --enable-examples \
<<<<<<< HEAD
=======
          --enable-doxygen \
>>>>>>> d8f74d5d3fb22f41808e9a1d19ad46742ca33476
          --build-3rdparty=libuv,libatomic_ops,openfec,pulseaudio,cpputest \
          --compiler=$c \
          test
done
