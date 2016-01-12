#! /bin/bash
set -xe
python26 /usr/bin/scons -Q clean
for v in debug release
do
  python26 /usr/bin/scons -Q \
    --with-3rdparty=uv,openfec,sox,gengetopt,cpputest variant=$v test
done
