#!/usr/bin/env bash

set -euxo pipefail

# "rebase.sh <branch>" is same as "git rebase <branch>", but it preserves
# original committer name, email, and date
# used to do periodic rebase of "develop" branch on "master"

exec='%s%nexec GIT_COMMITTER_DATE="%cD" GIT_COMMITTER_NAME="%cn" GIT_COMMITTER_EMAIL="%ce"'

GIT_EDITOR=: GIT_SEQUENCE_EDITOR=: \
    git -c rebase.instructionFormat="${exec} git commit --amend --no-edit" \
        rebase -i "$@"
