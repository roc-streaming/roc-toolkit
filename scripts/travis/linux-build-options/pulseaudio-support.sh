#! /bin/bash
set -euxo pipefail

scons -Q clean

scons -Q --enable-werror --build-3rdparty=all \
      --disable-pulseaudio \
      test

scons -Q --enable-werror --build-3rdparty=all \
      test

scons -Q --enable-werror --build-3rdparty=all \
      --enable-pulseaudio-modules \
      test
