#! /bin/bash
set -xe
scons clean
for c in gcc-4.4 gcc-4.6
do
  for v in debug release
  do
    scons --enable-werror --with-3rdparty=uv,openfec,cpputest compiler=$c variant=$v test
  done
done
