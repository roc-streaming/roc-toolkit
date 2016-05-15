#! /bin/bash
( set -xe;
  docker run --rm -ti -u "${UID}" -v "${TRAVIS_BUILD_DIR}":/opt/roc -w /opt/roc "$@" )
error=$?
if [ $error = 0 ]
then
  printf '%-32s \e[1;32m%s\e[m\n' "$1" "OK" >> build.status
else
  printf '%-32s \e[1;31m%s\e[m\n' "$1" "FAILED" >> build.status
fi
exit $error
