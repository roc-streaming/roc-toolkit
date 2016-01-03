#! /bin/bash
set -xe
scons clean
scons --enable-werror --with-3rdparty=all test
