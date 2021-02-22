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

set -ex
docker run --rm -t \
       -v android-sdk:/sdk \
       -v "$(pwd)":/opt/roc \
       -w /opt/roc \
       -e CI="${CI}" \
       --privileged \
       "${env[@]}" \
       rocstreaming/env-android \
       "$@"
