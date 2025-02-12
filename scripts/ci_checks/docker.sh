#!/usr/bin/env bash
#
# This script can be run locally by user 'bob' like this:
#
#   DOCKER_UID="$(id -u bob)" DOCKER_GID="$(id -g bob)" ./scripts/ci_checks/docker.sh ...
#
# (the full command line with exact arguments to docker.sh can be taken from CI)

set -eux -o pipefail

if [ -z "${CI:-}" ]; then
    opts=-ti
else
    opts=-t
fi

user="${DOCKER_UID:-"$UID"}"
if [ -n "${DOCKER_GID:-}" ]; then
    user="$user:$DOCKER_GID"
fi

docker run --rm ${opts} \
       --cap-add SYS_PTRACE \
       -u "$user" \
       -v "$(pwd)":/opt/roc \
       -w /opt/roc \
       -e CI="${CI:-}" \
       "$@"
