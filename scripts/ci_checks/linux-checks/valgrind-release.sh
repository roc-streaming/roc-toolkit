#!/usr/bin/env bash

set -euxo pipefail

scons -Q \
      --enable-werror \
      --enable-tests \
      --enable-examples \
      --compiler=gcc \
      --build-3rdparty=all

"$( dirname "$0" )"/run-tests-in-valgrind.sh
