#!/usr/bin/env bash

set -euxo pipefail

# debug: yes, debug-3rdparty: no
scons -Q --enable-werror --build-3rdparty=all \
      --enable-debug \
      --enable-tests \
      --enable-examples \
      test

# debug: yes, debug-3rdparty: yes
scons -Q --enable-werror --build-3rdparty=all \
      --enable-debug \
      --enable-debug-3rdparty \
      --enable-tests \
      --enable-examples \
      test
