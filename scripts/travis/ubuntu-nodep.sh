#! /bin/bash
set -xe
scons clean
scons --enable-werror --with-3rdparty=all test
scons --enable-werror --with-3rdparty=all --with-openfec=no test
