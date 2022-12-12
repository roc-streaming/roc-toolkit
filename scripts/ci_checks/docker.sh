#!/usr/bin/env bash
set -euxo pipefail

if [ -z "${CI:-}" ]; then
    opts=-ti
else
    opts=-t
fi

docker run --rm ${opts} \
       --cap-add SYS_PTRACE \
       -u "${UID}" \
       -v "$(pwd)":/opt/roc \
       -w /opt/roc \
       -e CI="${CI:-}" \
       "$@"
