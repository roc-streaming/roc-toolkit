#! /bin/bash
set -xe
scons -Q clean
scons -Q --enable-werror --with-3rdparty=all test
scons -Q --enable-werror --with-3rdparty=all --with-openfec=no test
