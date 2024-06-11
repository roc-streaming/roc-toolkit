#! /usr/bin/env bash

set -euo pipefail

cd "$(dirname "$0")"/..

docker run \
   --rm -t -u "${UID}" -v "$(pwd):$(pwd)" -w "$(pwd)" \
   rocstreaming/env-sphinx \
       scons --enable-sphinx --enable-doxygen docs
