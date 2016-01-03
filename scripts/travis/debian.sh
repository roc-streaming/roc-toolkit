#! /bin/bash
set -xe
scons clean
scons --enable-werror --with-3rdparty=uv,openfec,cpputest variant=debug test
scons --enable-werror --with-3rdparty=uv,openfec,cpputest variant=release test
