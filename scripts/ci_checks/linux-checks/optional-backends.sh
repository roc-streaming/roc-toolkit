#! /bin/bash
set -euxo pipefail

# pulseaudio:no sox:no
scons -Q --enable-werror --build-3rdparty=all \
      --disable-pulseaudio \
      --disable-sox \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
      test

# pulseaudio:no sox:yes
scons -Q --enable-werror --build-3rdparty=all \
      --disable-pulseaudio \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
      test

# pulseaudio:yes sox:no
scons -Q --enable-werror --build-3rdparty=all \
      --disable-sox \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
      test
