#! /bin/bash
set -euxo pipefail

scons -Q clean

for c in gcc-6 clang-6.0
do
    scons -Q \
          --enable-werror \
          --enable-pulseaudio-modules \
          --build-3rdparty=openfec,pulseaudio \
          --compiler=$c \
          test
done
