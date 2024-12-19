#!/usr/bin/env bash

set -eux -o pipefail

find bin/x86_64-pc-linux-gnu -name 'roc-test-*' |\
    while read tst
    do
        python3 scripts/scons_helpers/timeout-run.py 3000 \
            valgrind \
                --max-stackframe=10475520 \
                --error-exitcode=1 --exit-on-first-error=yes \
                ${tst}
    done
