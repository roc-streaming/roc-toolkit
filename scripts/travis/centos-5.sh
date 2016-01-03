#! /bin/bash
set -xe
python26 /usr/bin/scons clean
for v in debug release
do
  python26 /usr/bin/scons \
    --with-3rdparty=uv,openfec,sox,gengetopt,cpputest variant=$v test
done
