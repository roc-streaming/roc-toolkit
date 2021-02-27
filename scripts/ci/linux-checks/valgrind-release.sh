#! /bin/bash
set -euxo pipefail

scons -Q \
      --enable-werror \
      --enable-tests \
      --enable-benchmarks \
      --enable-examples \
      --build-3rdparty=all \
      test

find bin/x86_64-pc-linux-gnu -name 'roc-test-*' |\
    while read t
    do
        python2 scripts/build/timeout.py 300 \
            valgrind \
                --max-stackframe=10475520 \
                --error-exitcode=1 --exit-on-first-error=yes \
                $t
    done
