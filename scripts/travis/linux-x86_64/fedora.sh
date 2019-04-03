#! /bin/bash
set -xe
scons -Q clean
scons -Q --enable-werror --build-3rdparty=openfec,cpputest test
