#! /bin/bash
set -x

apt-get update
apt-get install -y g++ pkg-config scons gengetopt doxygen graphviz libsox-dev
apt-get install -y libtool autoconf automake make cmake

scons clean
scons --enable-werror --with-3rdparty=uv,openfec,cpputest variant=debug test
scons --enable-werror --with-3rdparty=uv,openfec,cpputest variant=release test
