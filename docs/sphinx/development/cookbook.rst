Cookbook
********

Clean build results
===================

.. code::

   $ scons -Q -c

or:

.. code::

   $ scons -Q clean

Run tests
=========

Build and run all tests:

.. code::

   $ scons -Q test

Run test for a specified module:

.. code::

   $ scons -Q test/roc_core

Run test for a specified module manually:

.. code::

   $ ./bin/x86_64-pc-linux-gnu/roc-test-core -v

Run single test:

.. code::

   $ ./bin/x86_64-pc-linux-gnu/roc-test-core -v -g array -n empty

Run linter
==========

Requires clang-tidy.

.. code::

   $ scons -Q tidy

Format code
===========

Requires clang-format.

.. code::

   $ scons -Q fmt

Build documentation
===================

Requires Doxygen, Sphinx, and Sphinx Breathe.

.. code::

   $ scons -Q doxygen sphinx

Run doxygen manually:

.. code::

   $ cd src/modules
   $ doxygen

and:

.. code::

   $ cd src/lib
   $ doxygen
