#! /bin/bash
run() {
    echo "# $*" && "$@" || exit 1
}

run dnf install -y gcc-c++ pkgconfig scons gengetopt doxygen graphviz libuv-devel sox-devel
run dnf install -y make cmake

run scons clean
run scons --enable-werror --with-3rdparty=openfec,cpputest variant=debug test
run scons --enable-werror --with-3rdparty=openfec,cpputest variant=release test
