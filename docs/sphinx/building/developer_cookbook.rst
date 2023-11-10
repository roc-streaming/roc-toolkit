Developer cookbook
******************

.. contents:: Table of contents:
   :local:
   :depth: 1

Build options
=============

Full developer build:

.. code::

    $ scons -Q --build-3rdparty=... \
      --enable-werror --enable-debug --enable-tests --enable-benchmarks \
      --enable-examples --enable-doxygen test bench

Explanation:

* ``-Q`` enables compact colored build output
* ``--build-3rdparty`` specifies the list of dependencies to be downloaded and built automatically
* ``--enable-werror`` turns compiler and Doxygen warnings into error (CI requires no warnings)
* ``--enable-debug`` enables debug build
* ``--enable-tests`` enables building of unit tests (CI requires all tests to pass)
* ``--enable-benchmarks`` enables building of micro benchmarks (not needed most time)
* ``--enable-examples`` enables building of API usage examples (not needed most time)
* ``--enable-doxygen`` enables running Doxygen and producing warnings for undocumented members (CI requires no warnings)
* ``test`` is the target to run unit tests
* ``bench`` is the target to run micro benchmarks

For ``--build-3rdparty`` option, see :doc:`/building/user_cookbook`.

For developer build, you may want to automatically download and build CppUTest (for unit tests) and Google Benchmark (for micro behcmarks):

.. code::

    $ scons -Q --build-3rdparty=...,cpputest,google-benchmark ...

Additionally, you can enable GCC/Clang sanitizers:

.. code::

    $ scons -Q --sanitizers=undefined,address ...
    $ scons -Q --sanitizers=all ...

Minimal build (don't build library and tools):

.. code::

    $ scons -Q --build-3rdparty=... --disable-lib --disable-tools

Disable specific dependencies (and features they provide):

.. code::

    $ scons -Q --build-3rdparty=... --disable-libunwind --disable-openfec \
      --disable-speexdsp --disable-sox --disable-pulseaudio

Compiler options
================

Select specific compiler:

.. code::

    $ scons -Q --compiler=gcc ...
    $ scons -Q --compiler=gcc-4.8 ...
    $ scons -Q --compiler=gcc-4.8.5 ...

Select toolchain for cross-compiling:

.. code::

    # arm-linux-gnueabihf-g++ should be in PATH
    $ scons -Q --host=arm-linux-gnueabihf ...

Select both compiler and toolchain:

.. code::

    # arm-linux-gnueabihf-clang++ should be in PATH
    $ scons -Q --compiler=clang --host=arm-linux-gnueabihf ...

Specify search paths manually:

.. code::

    $ scons -Q --with-openfec-includes=... --with-includes=... --with-libraries=...

Specify tools and flags manually:

.. code::

    $ scons -Q CXX="..." CXXFLAGS="..." ...

or:

.. code::

    $ export CXX="..."
    $ export CXXFLAGS="..."
    $ scons -Q ...

The full list of the available options and variables is documented in :doc:`/building/scons_options`.

Compiler launcher
=================

To speed up fresh build (i.e. subsequent builds after full clean), you can enable `ccache <https://ccache.dev/>`_.

.. code::

    $ scons -Q --compiler-launcher=ccache ...

Here, ``--compiler-launcher`` option defines launcher program that should be used for C and C++ compilers. For example, ``gcc <args>`` will be replaced with ``ccache gcc <args>``.

When ``--build-3rdparty`` is used, the specified launcher will be passed to third-party libraries as well.

macOS options
=============

There are a few macOS-specific build options:

* ``--macos-platform`` - specify macOS target platform version, a.k.a. macOS deployment target, for example ``10.12``.

  Resulting binaries will be compatible with all OS versions starting from the specified one, even if you're compiling on a different version. This requires all Roc dependencies to be built with the the same deployment target too. If you're using ``--build-3rdparty`` to build dependencies, deployment target will be automatically propagated to them.

* ``--macos-arch`` - specify macOS target architecture(s), for example ``x86_64`` or ``arm64``.

  You can specify multiple architectures (comma-separated) to produce universal binaries (a.k.a. fat binaries) that contain code for every architecture and can be executed on each of them. Use special architecture ``all`` to enable all supported architectures.

Building dependencies
=====================

Download and build selected dependencies, then build everything:

.. code::

    $ scons -Q --build-3rdparty=libuv:1.4.2,libunwind,openfec,cpputest ...

Download and build all dependencies, then build everything:

.. code::

    $ scons -Q --build-3rdparty=all

Per-module targets
==================

Build one module:

.. code::

    $ scons -Q ... roc_core

Run tests for one module:

.. code::

   $ scons -Q ... test/roc_core

Run benchmarks for one module:

.. code::

   $ scons -Q ... bench/roc_core

Running tests manually
======================

Run tests for the module manually:

.. code::

   $ ./bin/x86_64-pc-linux-gnu/roc-test-pipeline -v

Run a single test group:

.. code::

   $ ./bin/x86_64-pc-linux-gnu/roc-test-pipeline -v -g receiver_source

Run a single test:

.. code::

   $ ./bin/x86_64-pc-linux-gnu/roc-test-pipeline -v -g receiver_source -n one_session_long_run

Enable trace logging:

.. code::

   $ ./bin/x86_64-pc-linux-gnu/roc-test-core -t

Run benchmarks for the module manually:

.. code::

   $ ./bin/x86_64-pc-linux-gnu/roc-bench-pipeline

Formatting code
===============

Format code. Requires clang-format:

.. code::

   $ scons -Q fmt

Building documentation
======================

Build all documentation. Requires doxygen, sphinx-build, and breathe-apidoc.

.. code::

   $ scons -Q --enable-werror --enable-doxygen --enable-sphinx docs

Or build specific parts of documentation:

.. code::

   $ scons -Q --enable-werror --enable-doxygen --enable-sphinx doxygen
   $ scons -Q --enable-werror --enable-doxygen --enable-sphinx sphinx

Remove generated documentation:

.. code::

   $ scons -Q cleandocs

Run doxygen manually:

.. code::

   # internal modules (HTML)
   $ cd src/internal_modules
   $ doxygen

   # public api (XML for sphinx)
   $ cd src/public_api
   $ doxygen

Cleaning build results
======================

Clean everything:

.. code::

   $ scons -Q -c

or:

.. code::

   $ scons -Q clean

Clean build results except third-parties and documentation:

.. code::

   $ scons -Q cleanbuild

Clean only built documentation:

.. code::

   $ scons -Q cleandocs
