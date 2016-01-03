#! /bin/bash
run() {
    echo "# $*" && "$@" || exit 1
}

run apt-get update
run apt-get install -y g++ pkg-config scons gengetopt doxygen graphviz libsox-dev
run apt-get install -y libtool autoconf automake make cmake

run scons clean
run scons --enable-werror --with-3rdparty=uv,openfec,cpputest variant=debug test
run scons --enable-werror --with-3rdparty=uv,openfec,cpputest variant=release test
