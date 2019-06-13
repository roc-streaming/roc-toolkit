#! /bin/bash
( set -x;
  docker run --rm -t \
         --cap-add SYS_PTRACE \
         -u "${UID}" \
         -v "${TRAVIS_BUILD_DIR:-`pwd`}":/opt/roc \
         -w /opt/roc "$@" )
error=$?
if [ $error = 0 ]
then
  printf '%-60s \e[1;32m%s\e[m\n' "$1" "OK" >> build.status
else
  printf '%-60s \e[1;31m%s\e[m\n' "$1" "FAILED" >> build.status
fi
exit $error
