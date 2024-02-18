#!/usr/bin/env bash

set -euxo pipefail

# debug
scons -Q \
      --enable-werror \
      --enable-debug \
      --enable-tests \
      --enable-examples \
      --compiler=clang \
      --build-3rdparty=all \
      test

find bin/x86_64-pc-linux-gnu -name 'roc-test-*' |\
    while read tst
    do
        python3 scripts/scons_helpers/timeout-run.py 300 \
            valgrind \
                --max-stackframe=10475520 \
                --error-exitcode=1 --exit-on-first-error=yes \
                ${tst}
    done

# release
scons -Q \
      --enable-werror \
      --enable-tests \
      --enable-examples \
      --compiler=clang \
      --build-3rdparty=all \
      test

find bin/x86_64-pc-linux-gnu -name 'roc-test-*' |\
    while read tst
    do
        python3 scripts/scons_helpers/timeout-run.py 300 \
            valgrind \
                --max-stackframe=10475520 \
                --error-exitcode=1 --exit-on-first-error=yes \
                ${tst}
    done
