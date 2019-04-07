#! /bin/bash
set -euxo pipefail

scons -Q clean

scons -Q --enable-werror --build-3rdparty=openfec test

for c in gcc-4.8 gcc-5 clang-3.7
do
    scons -Q \
          --enable-werror \
          --enable-pulseaudio-modules \
          --build-3rdparty=openfec,pulseaudio \
          --compiler=$c \
          test
done
