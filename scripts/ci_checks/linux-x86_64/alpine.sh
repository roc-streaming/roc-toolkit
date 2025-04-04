#!/usr/bin/env bash

set -euxo pipefail

scons -Q \
      --enable-werror \
      --enable-tests \
      --enable-examples \
      --build-3rdparty=openfec \
      test
