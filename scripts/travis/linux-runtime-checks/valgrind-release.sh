#! /bin/bash
set -euxo pipefail

scons -Q clean
scons -Q --enable-werror --build-3rdparty=all

find bin/x86_64-pc-linux-gnu -name 'roc-test-*' |\
    while read t
    do
        python2 scripts/wrappers/timeout.py 300 \
            valgrind \
                --max-stackframe=10475520 \
                --error-exitcode=1 --exit-on-first-error=yes \
                $t
    done
