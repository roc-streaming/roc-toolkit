SCons options
*************

.. contents:: Table of contents:
   :local:
   :depth: 1

Usage
=====

.. code::

    $ scons --help
    $ scons [options] [variable1=value1 variable2=value2 ...] [target1 target2 ...]

Build results
=============

Build results are installed into the following directories:

- ``./bin/<host>``
    Roc tools, libraries, and PulseAudio modules

- ``./3rdparty/<host>/rpath``
    automatically built 3rd-party shared libraries

- ``./docs/man``
    Manual pages

- ``./docs/html``
    HTML documentation

Options
=======

  -Q                          compact colored output
  -c                          remove build results
  -n                          dry run
  -j N, --jobs=N              allow N parallel jobs at once

  --prefix=PREFIX             installation prefix, /usr by default
  --build=BUILD               system name where Roc is being compiled, e.g.
                                'x86_64-pc-linux-gnu', auto-detect if empty
  --host=HOST                 system name where Roc will run, e.g.
                                'arm-linux-gnueabihf', equal to --build if
                                empty
  --platform=PLATFORM         platform name where Roc will run, supported
                                values: empty (detect from host), 'linux',
                                'darwin'
  --compiler=COMPILER         compiler name and optional version, e.g.
                                'gcc-4.9', supported names: empty (detect what
                                available), 'gcc', 'clang'
  --sanitizers=SANITIZERS     list of gcc/clang sanitizers, supported names:
                                '', 'all', 'undefined', 'address'
  --enable-debug              enable debug build
  --enable-debug-3rdparty     enable debug build for 3rdparty libraries
  --enable-werror             enable -Werror compiler option
  --enable-pulseaudio-modules
                              enable building of pulseaudio modules
  --disable-lib               disable libroc building
  --disable-tools             disable tools building
  --disable-tests             disable tests building
  --disable-examples          disable examples building
  --disable-doc               disable Doxygen documentation generation
  --disable-openfec           disable OpenFEC support required for FEC codes
  --with-pulseaudio=WITH_PULSEAUDIO
                              path to the fully built pulseaudio source
                                directory used when building pulseaudio
                                modules
  --build-3rdparty=BUILD_3RDPARTY
                              download and build specified 3rdparty libraries,
                                pass a comma-separated list of library names
                                and optional versions, e.g. 'uv:1.4.2,openfec'
  --override-targets=OVERRIDE_TARGETS
                              override targets to use, pass a comma-separated
                                list of target names, e.g.
                                'gnu,posix,uv,openfec,...'

Variables
=========

- CFLAGS
- CXXFLAGS
- LDFLAGS
- CC
- CXX
- LD
- AR
- RANLIB
- GENGETOPT
- DOXYGEN
- SPHINX_BUILD
- BREATHE_APIDOC
- PKG_CONFIG

Targets
=======

`omitted`
    build everything

``test``
    build everything and run tests

``clean``
    remove build results

``install``
    install build results into the system

``uninstall``
    remove build results from the system

``fmt``
    format source code (requires clang-format)

``tidy``
    run linter (requires clang-tidy)

``docs``
    build website (includes ``doxygen`` and ``sphinx`` targets)

``doxygen``
    build doxygen documentation (requires doxygen and graphviz)

``sphinx``
    build sphinx documentation (requires doxygen, sphinx-build, and breathe)

``{module}``
    build specific module, e.g. ``roc_core``

``test/{module}``
    run tests for specific module, e.g. ``test/roc_core``
