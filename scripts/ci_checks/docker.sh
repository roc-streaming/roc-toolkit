#!/usr/bin/env bash
#
# This script can be run locally by user 'bob' like this:
#
#   DOCKER_UID="$(id -u bob)" DOCKER_GID="$(id -g bob)" ./scripts/ci_checks/docker.sh ...
#
# (the full command line with exact arguments to docker.sh can be taken from CI)

set -eu -o pipefail

user="${DOCKER_UID:-${UID:-`id -u`}}:${DOCKER_GID:-${GID:-`id -g`}}"
pwd="$(pwd)"

if [ -z "${CI:-}" ]; then
    opts=-ti
else
    opts=-t
fi

set -x
docker run --rm ${opts} \
       --cap-add SYS_PTRACE \
       -u "$user" \
       -v "$pwd:/opt/roc" \
       -w "/opt/roc" \
       -e CI="${CI:-}" \
       "$@"
