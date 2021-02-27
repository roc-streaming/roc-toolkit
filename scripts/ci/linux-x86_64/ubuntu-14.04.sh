#! /bin/bash
set -euxo pipefail

for c in gcc-4.4 clang-3.4
do
    scons -Q \
          --enable-werror \
          --enable-pulseaudio-modules \
          --enable-tests \
          --enable-examples \
          --enable-doxygen \
          --build-3rdparty=libuv,libatomic_ops,openfec,pulseaudio,cpputest \
          --compiler=$c \
          test
done
