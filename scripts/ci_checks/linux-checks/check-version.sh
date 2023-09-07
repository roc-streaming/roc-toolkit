#!/usr/bin/env bash

set -euo pipefail


tagname="$(git tag -l | tail -n1)"

header_tagname="v$(python3 scripts/scons_helpers/parse-version.py)"

if [[ "$tagname" != "$header_tagname" ]]; then
  echo >&2 
  echo >&2"version check FAILED"
  echo >&2"the git tag name and version declared in version.h is not the same"
  echo >&2"${tagname} git version tag"
  echo >&2"${header_tagname} header version tag"
  echo >&2
  exit 1
fi

echo "version check SUCCEEDED"
echo 

