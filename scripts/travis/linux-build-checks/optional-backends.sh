#! /bin/bash
set -euxo pipefail

scons -Q clean

scons -Q --enable-werror --build-3rdparty=all \
      --disable-pulseaudio \
      --disable-sox \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
      test

scons -Q --enable-werror --build-3rdparty=all \
      --disable-pulseaudio \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
      test

scons -Q --enable-werror --build-3rdparty=all \
      --disable-sox \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
      test

scons -Q --enable-werror --build-3rdparty=all \
      --enable-pulseaudio-modules \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
      test
