#! /bin/bash
set -xe
python26 /usr/bin/scons -Q clean
python26 /usr/bin/scons -Q --with-3rdparty=uv,openfec,sox,gengetopt,cpputest test
