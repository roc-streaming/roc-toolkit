#! /bin/bash
set -euxo pipefail

# disable optional dependencies, don't build optional targets
scons -Q --enable-werror --build-3rdparty=all \
      --disable-shared \
      --disable-tools \
      --disable-c11 \
      --disable-libunwind \
      --disable-openfec \
      --disable-speex \
      --disable-sox \
      --disable-pulseaudio

# disable optional dependencies, build all targets
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

# enable optional dependencies, build all targets
scons -Q --enable-werror --build-3rdparty=all \
      --enable-static \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
      test
