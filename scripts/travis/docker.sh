#! /bin/bash
set -xe
docker run --rm -ti -v "${TRAVIS_BUILD_DIR}":/tmp/roc -w /tmp/roc "$@"
