#! /bin/bash
set -euxo pipefail
scons -Q clean
scons -Q --enable-werror --build-3rdparty=libuv,openfec,cpputest test
