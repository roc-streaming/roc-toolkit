Version control
***************

Branches
========

The following branches are used:

* ``master`` -- Stable and ready-to-use branch for users. Should be tested on all supported platforms. Public releases are tagged from this branch.

* ``develop`` -- Unstable branch for developers. May not be fully tested. From time to time it is considered ready and merged into the master branch.

* ``feature/xxx`` -- Unstable feature branches for developers. Created for large work-in-progress features to allow the usual continuous integration and code review process without breaking the develop branch. Merged into develop when becomes ready.

Releases
========

See `Releases <https://github.com/roc-streaming/roc-toolkit/releases>`_ and :doc:`/development/changelog` pages for the list of releases and their detailed description.

Releases are tagged from the master branch and named according to the `semantic versioning <https://semver.org/>`_ rules:

* patch version Z (x.y.Z) is incremented when introducing backwards-compatible bug fixes;
* minor version Y (x.Y.z) is incremented when introducing backwards-compatible features;
* major version X (X.y.z) is incremented when introducing backwards-incompatible changes.

Prior to 1.0.0 release, which hasn't happened yet, there is no compatibility promise for the public API. Small breaking changes are possible in any release.

History
=======

History is always kept linear in the master and develop branches.

The develop branch should be rebased on the master branch, and the feature branches and pull requests should be rebased on the develop branch. Not fast-forward merges into master and develop are not allowed.

Pull requests
=============

All pull-requests should be targeted on the develop branch. To be merged, a pull request should be rebased on the latest commit from that branch.

It's recommended to group independent changes, like formatting, refactoring, bug fixes, and new features into separate commits. Bonus points if build and tests pass on every commit. This helps a lot when bisecting a regression.

A pull request should pass the  :doc:`continuous integration </development/continuous_integration>` checks and code review to be merged.
