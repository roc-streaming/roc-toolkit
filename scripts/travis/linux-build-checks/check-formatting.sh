#! /bin/bash
set -euo pipefail

scons -Q fmt

diff=$(git status --porcelain)

if [[ $diff ]]; then
    echo
    echo "format check FAILED"
    echo "please install clang-format and run scons -Q fmt"
    echo
    GIT_PAGER= git diff
    exit 1
else
    echo "format check OK"
fi
