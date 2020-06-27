#! /bin/bash
set -euxo pipefail

scons -Q clean

scons -Q \
      --enable-werror \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
<<<<<<< HEAD
=======
      --enable-doxygen \
>>>>>>> d8f74d5d3fb22f41808e9a1d19ad46742ca33476
      --build-3rdparty=openfec,cpputest \
      test
