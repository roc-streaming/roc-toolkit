#! /bin/bash
set -xe
scons -Q clean
scons -Q --enable-werror --with-3rdparty=openfec,cpputest variant=debug test
scons -Q --enable-werror --with-3rdparty=openfec,cpputest variant=release test
