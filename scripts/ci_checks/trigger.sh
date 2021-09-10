#! /bin/bash
set -euxo pipefail

repo="$1"
branch="$2"
token="$3"

curl -s -X POST \
     "https://api.github.com/repos/${repo}/dispatches" \
     -H 'Accept: application/vnd.github.everest-preview+json' \
     -u "${token}" \
     --data '{"event_type": "Trigger", "client_payload": { "branch": "'"${branch}"'" }}'
