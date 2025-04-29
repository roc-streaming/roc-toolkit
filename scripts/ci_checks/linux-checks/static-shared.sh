#!/usr/bin/env bash

set -euxo pipefail

# shared: no, static: no, tests: no
scons -Q --enable-werror --build-3rdparty=all \
      --disable-shared

# shared: no, static: no, tests: yes
scons -Q --enable-werror --build-3rdparty=all \
      --disable-shared \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
      test

# shared: yes, static: no, tests: yes
scons -Q --enable-werror --build-3rdparty=all \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
      test

# shared: no, static: yes, tests: yes
scons -Q --enable-werror --build-3rdparty=all \
      --disable-shared \
      --enable-static \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
      test

# shared: yes, static: yes, tests: yes
scons -Q --enable-werror --build-3rdparty=all \
      --enable-static \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
      test
