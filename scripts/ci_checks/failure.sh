#!/usr/bin/env bash
set -euo pipefail

env

cat config.log || true

if [ -d .sconf_temp ]; then
    for f in .sconf_temp/*.c; do
        echo
        echo $f
        cat $f || true
    done
fi
