#! /bin/bash
set -euxo pipefail
curl -s -X POST \
  -H "Content-Type: application/json" \
  -H "Accept: application/json" \
  -H "Travis-API-Version: 3" \
  -H "Authorization: token ${TRAVIS_TOKEN}" \
  -d '{"request":{"branch":"'"$2"'"}}' \
  "https://api.travis-ci.com/repo/$1/requests"
