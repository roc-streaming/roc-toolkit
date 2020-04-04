Developer cookbook
******************

.. contents:: Table of contents:
   :local:
   :depth: 1

Clean
=====

.. code::

   $ scons -Q -c

or:

.. code::

   $ scons -Q clean

Build
=====

Build all:

.. code::

   $ scons -Q

Build one module:

.. code::

    $ scons -Q roc_core

Minimal build:

.. code::

    $ scons -Q --disable-openfec --disable-tools --disable-tests --disable-examples --disable-doc

Developer build:

.. code::

    $ scons -Q --enable-werror --enable-debug --sanitizers=all

Tests
=====

Build and run all tests:

.. code::

   $ scons -Q test

Run tests for the specified module:

.. code::

   $ scons -Q test/roc_core

Run tests for the module manually:

.. code::

   $ ./bin/x86_64-pc-linux-gnu/roc-test-core -v

Run single test:

.. code::

   $ ./bin/x86_64-pc-linux-gnu/roc-test-core -v -g array -n empty

Compiler options
================

Select compiler:

.. code::

    $ scons -Q --compiler=gcc
    $ scons -Q --compiler=gcc-4.8
    $ scons -Q --compiler=gcc-4.8.5

Select toolchain:

.. code::

    $ scons -Q --host=arm-linux-gnueabihf

Set tools and options manually:

.. code::

    $ scons -Q CXX="..." CXXFLAGS="..." ...

Dependencies
============

Download and build selected dependencies, then build everything:

.. code::

    $ scons -Q --build-3rdparty=libuv:1.4.2,libunwind,openfec,cpputest

Download and build all dependencies, then build everything:

.. code::

    $ scons -Q --build-3rdparty=all

Documentation
=============

Build all documentation. Requires doxygen, sphinx, and breathe.

.. code::

   $ scons -Q docs

Build specific parts of documentation:

.. code::

   $ scons -Q doxygen
   $ scons -Q sphinx

Remove generated documentation:

.. code::

   $ scons -Q cleandocs

Run doxygen manually:

.. code::

   # internal modules
   $ cd src/modules
   $ doxygen

   # public api
   $ cd src/lib
   $ doxygen

Format code
===========

Format code. Requires clang-format.

.. code::

   $ scons -Q fmt

Linter
======

Run linter. Requires clang-tidy.

.. code::

   $ scons -Q tidy
