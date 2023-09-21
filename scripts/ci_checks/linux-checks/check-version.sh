#!/usr/bin/env bash

set -euo pipefail

tagname="$(git tag -l | tail -n1)"

header_tagname="v$(python3 scripts/scons_helpers/parse-version.py)"

if [[ "$tagname" != "$header_tagname" ]]; then
  echo
  echo "version check FAILED"
  echo "the git tag name and version declared in version.h is not the same"
  echo "${tagname} git version tag"
  echo "${header_tagname} header version tag"
  echo
  exit 1
fi

echo "version check SUCCEEDED"
echo
