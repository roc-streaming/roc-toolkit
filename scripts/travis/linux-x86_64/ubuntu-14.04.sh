#! /bin/bash
set -euxo pipefail

scons -Q clean

for c in gcc-4.4 gcc-4.6 clang-3.4
do
    scons -Q \
          --enable-werror \
          --enable-pulseaudio-modules \
          --build-3rdparty=libuv,openfec,pulseaudio,cpputest \
          --compiler=$c \
          test
done
