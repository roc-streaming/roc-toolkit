#!/usr/bin/env bash

set -euo pipefail


tagname=$(git tag -l)
tagname=${tagname: -5}
header_tagname=$(python3 scripts/scons_helpers/parse-version.py)

if [[ "$tagname" != "$header_tagname" ]]; then
      echo
      echo "version check FAILED"
      echo "the git tag name and version declared in version.h is not the same"
      echo "${tagname} git version tag"
      echo "${header_tagname} header version tag"
      echo
      exit 1
  else
     echo "version check OK"
fi