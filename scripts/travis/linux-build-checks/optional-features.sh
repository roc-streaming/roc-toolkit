#! /bin/bash
set -euxo pipefail

scons -Q clean

scons -Q --enable-werror --build-3rdparty=all \
      --disable-lib \
      --disable-tools \
      --disable-doc \
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
