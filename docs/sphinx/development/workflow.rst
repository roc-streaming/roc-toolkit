Development workflow
********************

.. contents:: Table of contents:
   :local:
   :depth: 1

Branches
========

The following branches are used:

* ``master`` -- Stable and ready-to-use branch **for users**. Merged to this branch are tested on all supported platforms. Public releases are tagged from this branch.

* ``develop`` -- Unstable branch **for developers**. Is not regularly tested besides CI and may be sometimes broken. From time to time it is considered ready and merged into the master branch.

Releases
========

See `Releases <https://github.com/roc-streaming/roc-toolkit/releases>`_ and :doc:`/development/changelog` pages for the list of releases and their detailed description.

Releases are **tagged from master** branch and named according to the `semantic versioning <https://semver.org/>`_ rules:

* patch version Z (x.y.Z) is incremented when introducing backwards-compatible bug fixes;
* minor version Y (x.Y.z) is incremented when introducing backwards-compatible features;
* major version X (X.y.z) is incremented when introducing backwards-incompatible changes.

Prior to 1.0.0 release, which hasn't happened yet, there is no compatibility promise for the public API. Small breaking changes are possible in any release.

History
=======

History in ``master`` and ``develop`` branches is **kept linear**. Only fast-forward merges and rebases are used.

New changes always reach ``develop`` branch first. Sometimes, specific commits may be cherry-picked from ``develop`` to ``master`` if quick bug-fix release is needed. Eventually ``develop`` branch is rebased on ``master``, and ``master`` is fast-forwarded to ``develop`` branch head. Usually this is done before release.

Pull requests
=============

Please follow a few simple rules to ease maintainers work:

* Pull request should be **targeted to develop** branch and rebased on it.

* Add a **reference to the issue** to pull request description.

* Until pull request is ready for review, **mark it draft**.

* When pull request becomes ready, use GitHub **request review** feature.

* When you submit updated version after review, don't forget to **re-request review**.

* When you adderess issues raised during review, please **don't resolve discussions** by yourself. Instead, leave a comment or thumbs up on that discussion.

It's recommended to group independent changes, like formatting, refactoring, bug fixes, and new features into separate commits. Bonus points if build and tests pass on every commit. This helps a lot when bisecting a regression.

Before submitting PR, don't forget to run code formatting, as described in :doc:`coding guidelines </development/coding_guidelines>`. After submitting, ensure that all :doc:`continuous integration </development/continuous_integration>` checks pass on your PR and fix them if needed.

Review cycle
============

The following labels are used during code review to indicate pull request state:

- ``work in progress`` -- author is doing changes and maintainer should not do review
- ``ready for review`` -- author have finished doing changes and requests review from maintaner
- ``review in progress`` -- maintainer started doing review; used when review can take long time, e.g. a few days
- ``needs revision`` -- maintainer finished review and requested updates or clarifications from author
- ``needs rebase`` -- automatically added when CI detects that there are unresolved conflicts; author should rebase PR on fresh develop branch

These labels are usually assigned automatically by GitHub actions, or manually by maintainer.
