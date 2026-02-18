#!/usr/bin/env bash

set -eux -o pipefail

export RUNNING_IN_VALGRIND=1

find bin/x86_64-pc-linux-gnu -name 'roc-test-*' |\
    while read tst
    do
        python3 scripts/scons_helpers/timeout-run.py 900 \
            valgrind \
                --suppressions=src/valgrind_suppressions.supp \
                --max-stackframe=10475520 \
                --error-exitcode=1 --exit-on-first-error=yes \
                ${tst}
    done
