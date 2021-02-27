#! /bin/bash
set -euxo pipefail

scons -Q \
      --enable-werror \
      --enable-tests \
      --enable-examples \
      --build-3rdparty=libuv,libatomic_ops,openfec,cpputest \
      test
