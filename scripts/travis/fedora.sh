#! /bin/bash
set -x

dnf install -y gcc-c++ pkgconfig scons gengetopt doxygen graphviz libuv-devel sox-devel
dnf install -y make cmake

scons clean
scons --enable-werror --with-3rdparty=openfec,cpputest variant=debug test
scons --enable-werror --with-3rdparty=openfec,cpputest variant=release test
