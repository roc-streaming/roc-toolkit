#!/usr/bin/env bash
set -euo pipefail

if [ -e config.log ]; then
    cat config.log
fi

if [ -d .sconf_temp ]; then
    for f in .sconf_temp/*.c; do
        echo
        echo $f
        cat $f
    done
fi
