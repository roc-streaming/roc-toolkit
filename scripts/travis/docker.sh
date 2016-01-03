#! /bin/bash
set -xe
docker run --rm -ti -u "${UID}" -v "${TRAVIS_BUILD_DIR}":/tmp/roc -w /tmp/roc "$@"
