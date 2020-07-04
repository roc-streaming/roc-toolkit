#! /bin/bash
name="$(echo "$*" | sed -re 's:\S+/([^/]+):\1:')"
env=( )
while :
do
    case "$1" in
        *=*)
            env+=( -e "$1" )
            shift
            ;;
        *)
            break
            ;;
    esac
done
( set -x;
  docker run --rm -t \
         -v android-sdk:/sdk \
         -v "${TRAVIS_BUILD_DIR:-`pwd`}":/opt/roc \
         -w /opt/roc \
         "${env[@]}" \
         rocstreaming/env-android \
         "$@" )
error=$?
if [ $error = 0 ]
then
  printf '%-90s \e[1;32m%s\e[m\n' "$name" "OK" >> build.status
else
  printf '%-90s \e[1;31m%s\e[m\n' "$name" "FAILED" >> build.status
fi
exit $error
