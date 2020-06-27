#! /bin/bash
set -euxo pipefail

scons -Q clean

scons -Q --enable-werror --build-3rdparty=all \
      --disable-lib \
      --disable-tools \
<<<<<<< HEAD
      --disable-doc \
=======
>>>>>>> d8f74d5d3fb22f41808e9a1d19ad46742ca33476
      --disable-c11 \
      --disable-libunwind \
      --disable-openfec \
      --disable-speex \
      --disable-sox \
      --disable-pulseaudio

scons -Q --enable-werror --build-3rdparty=all \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
      --disable-libunwind \
      --disable-openfec \
      --disable-speex \
      --disable-sox \
      --disable-pulseaudio \
      test

scons -Q --enable-werror --build-3rdparty=all \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
      test
