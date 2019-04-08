Guidelines
**********

.. contents:: Table of contents:
   :local:
   :depth: 1

Design
======

Dependencies
------------

We avoid unnecessary external dependencies. Optional dependencies are preferred when it makes sense. Cross-platform dependencies are preferred when possible. We don't use STL and general purpose libraries like Boost. Instead, we maintain our own minimal utility set designed to be very lightweight and allowing fine-grained memory management.

Portability
-----------

The main codebase is portable. Platform-specific components are isolated inside :ref:`target_ <targets>` directories. This method is usually preferred over using preprocessor. We use C++98 for the main codebase. Compiler-specific code should be isolated inside target directories as well.

Tests
-----

Although the test coverage is not 100%, non-trivial components are mostly covered with unit tests. Significant features are also covered with integration tests for sender and receiver pipelines. Public API is covered with integration tests as well.

Implementation
==============

Memory management
-----------------

All allocations should go through an allocator or an object pool. Pools are preferred when applicable. Utility classes like smart pointers and containers either don't perform allocations at all or do it only when explicitly requested. In most cases, RAII is preferred to manage objects lifetime.

Threads
-------

In most cases, it is preferred to restrict communications between threads and avoid sharing ownership of mutable objects between multiple threads. It's often acceptable to place responsibility of inter-thread communication at higher abstraction levels, allowing lower levels to live in ignorance of other threads.

Error handling
--------------

We don't use exceptions and error codes. If a component's contract is broken, it should terminate the program gracefully using ``roc_panic()``. A panic is preferred over a crash, so it's recommended to validate contracts when possible. Other errors should be logged carefully using ``roc_log()`` and reported to the upper level. Note that a panic should always mean a bug in Roc. It should not be used to validate data came from the outside world, e.g. from the public API user or from the network.

Code style
==========

Formatting
----------

The code is formatted using ``scons fmt``, which invokes ``clang-format`` (if installed) and ``scipts/format.py``. Formatting config can be found in ``.clang-format`` file. If ``clang-format`` gets crazy about your file, add it to ``.fmtignore``.

Warnings
--------

The code should build with ``--enable-werror`` on recent GCC and Clang versions, thought some annoying warnings can be disabled in ``SConstruct``.

Comments
--------

Header files, classes, public members, and non-static functions should be documented using Doxygen. If Doxygen is installed, it is invoked during build and warns about undocumented items.

Copyrights
----------

Source files contain "Roc authors" copyright and MPL-2 header. ``scons fmt`` automatically adds them to all header files. The names of maintainers and contributors are listed in the ``AUTHORS`` file. Feel free to add your name to the list in your pull requests.

Version control
===============

Branches
--------

The following branches are used:

* master - Stable branch tested on all supported platforms and ready for users. Public releases are tagged from this branch.

* develop - Unstable branch where all actual development happens. From time to time it is considered ready and merged into the master branch.

* Custom branches may be created for large work-in-progress features to avoid breaking develop for a long time.

History is always kept linear in master and develop branches. Not fast-forward merges are not allowed.

Commits
-------

It's recommended to group independent changes, like formatting, refactoring, bug fixes, and new features into separate commits. Bonus points if build and tests pass on every commit. This helps a lot when bisecting a regression.

Pull requests
-------------

All pull-requests should be targeted on the develop branch. To be merged, a pull request should be rebased on the latest commit from that branch.
