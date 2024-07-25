Maintainer notes
****************

.. contents:: Table of contents:
   :local:
   :depth: 1

Merging pull request
====================

Pull requests should be merged using `rgh.py <https://github.com/roc-streaming/roc-toolkit/blob/develop/scripts/rgh.py>`_ script, which does the following:

- Rebases PR on up-to-date develop branch.
- If requested, squashes all commits into one.
- Overwrites commit messages to add a link to related issue to each commit (e.g. "Implement feature" becomes "gh-123: Implement feature"). The issue number should be present is PR description or passed as command-line flag.
- Force-pushes updated commits to PR's branch.
- Asks GitHub to merge PR.

You should choose whether to merge by rebasing or squashing.

Merge PR by rebasing:

.. code::

   scripts/rgh.py merge_pr --rebase 123

Merge PR by rebasing and squashing all commits into one:

.. code::

   scripts/rgh.py merge_pr --squash 123

If PR description doesn't have a link to issue, the script will complain and fail. It will also fail if the issue does not have a milestone assigned. You can manually specify it:

.. code::

   scripts/rgh.py merge_pr --rebase 123 --issue 456 --milestone 0.9.9

The script will use given issue for commits and also will update PR and issue.

When squashing, you can overwrite commit message using ``--title`` option.

Showing pull request info
=========================

Show PR information, useful to do before running ``merge_pr``:

.. code::

   scripts/rgh.py show_pr 123

Rebasing develop on master
==========================

This is usually done before making release. It's needed only if some commits were cherry-picked to ``master`` after ``develop`` was rebased last time.

Update branches:

.. code::

   git switch master && git pull origin master
   git switch develop && git pull origin develop

Rebase ``develop`` on ``master``, preserving git authors and committers:

.. code::

   scripts/rgh.py stb_rebase master

Push to your fork:

.. code::

   git push -f <your fork> develop

When CI on your fork passes, push to origin:

.. code::

   git push -f origin develop

Updating master to develop
==========================

Before doing this, first ensure that branches are up-to-date and ``develop`` is rebased on ``master``.

Then fast-forward ``master`` to ``develop``:

.. code::

   git switch master
   git merge --ff-only develop

Creating release
================

Rename ``next`` milestone to ``1.2.3`` and close it. Create new ``next`` milestone.

Add new release to :doc:`changelog page </development/changelog>`.

Update version number in `version header <https://github.com/roc-streaming/roc-toolkit/blob/develop/src/public_api/include/roc/version.h>`_.

Update authors page:

.. code::

   scripts/update_authors.sh

Update specs for debian and rpm packages:

.. code::

   scripts/update_packages.py

Create and push tag:

.. code::

   git tag v1.2.3
   git push origin v1.2.3

When CI passes, go to `releases page <https://github.com/roc-streaming/roc-toolkit/releases>`_, add links to changelog and milestone to release description, and publish release.

Post announcement to matrix and, in case of big releases, to the mailing list.
