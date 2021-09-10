#! /bin/bash
set -ex
docker run --rm -t \
       --cap-add SYS_PTRACE \
       -u "${UID}" \
       -v "$(pwd)":/opt/roc \
       -w /opt/roc \
       -e CI="${CI}" \
       "$@"
