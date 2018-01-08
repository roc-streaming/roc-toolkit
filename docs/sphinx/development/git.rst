Git
***

Branches
========

There are two main branches:

``master``

    Stable branch tested on all supported platforms and ready for users. Public releases are tagged commits in this branch.

``develop``

    Unstable branch where all actual development happens. When it is considered ready, it is merged into ``master``.

History is always kept linear in both branches, i.e. non-fast-forward merges are not allowed.

Additional branches are sometimes created for large work-in-progress features to avoid breaking ``develop`` for a long time.

Commits
=======

It's recommended to group independent changes, like formatting, refactoring, bug fixes, and new features into separate commits. Bonus points if build and tests pass on every commit.

Pull requests
=============

All pull-requests should be targeted on the ``develop`` branch. To be merged, a pull request should be rebased on the latest commit from ``develop`` branch (thought in many cases, GitHub can do it automatically during the merge).
