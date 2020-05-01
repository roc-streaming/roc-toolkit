#! /bin/bash
set -euxo pipefail

scons -Q clean

scons -Q \
      --enable-werror \
      --enable-tests \
      --enable-examples \
      --build-3rdparty=libuv,openfec,cpputest \
      test
