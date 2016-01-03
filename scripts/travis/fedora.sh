#! /bin/bash
set -xe
scons clean
scons --enable-werror --with-3rdparty=openfec,cpputest variant=debug test
scons --enable-werror --with-3rdparty=openfec,cpputest variant=release test
