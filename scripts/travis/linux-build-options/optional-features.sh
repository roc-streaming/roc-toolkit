#! /bin/bash
set -euxo pipefail

scons -Q clean

scons -Q --enable-werror --build-3rdparty=all \
      --disable-lib \
      --disable-tools \
      --disable-tests \
      --disable-examples \
      --disable-doc \
      --disable-libunwind \
      --disable-openfec \
      --disable-sox \
      --disable-pulseaudio

scons -Q --enable-werror --build-3rdparty=all \
      --disable-openfec \
      --disable-sox \
      --disable-pulseaudio \
      test

scons -Q --enable-werror --build-3rdparty=all \
      --disable-sox \
      test

scons -Q --enable-werror --build-3rdparty=all \
      test
