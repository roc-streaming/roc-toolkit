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

- ``./man``
    generated manual pages

- ``./html``
    generated HTML documentation

Options
=======

-Q                                                     compact colored output
-c                                                     remove build results
-n                                                     dry run
-j N, --jobs=N                                         allow N parallel jobs at once

--prefix=PREFIX                                        installation prefix, '/usr' by default
--bindir=BINDIR                                        path to the binary installation directory (where to install Roc command-line tools), '<prefix>/bin' by default
--libdir=LIBDIR                                        path to the library installation directory (where to install Roc library), auto-detect if empty
--incdir=INCDIR                                        path to the headers installation directory (where to install Roc headers), '<prefix>/include' by default
--mandir=MANDIR                                        path to the manuals installation directory (where to install Roc manual pages), '<prefix>/share/man/man1' by default
--pulseaudio-module-dir=PULSEAUDIO_MODULE_DIR          path to the PulseAudio modules installation directory (where to install Roc PulseAudio modules), auto-detect if empty
--build=BUILD                                          system name where Roc is being compiled, e.g. 'x86_64-pc-linux-gnu', auto-detect if empty
--host=HOST                                            system name where Roc will run, e.g. 'arm-linux-gnueabihf', auto-detect if empty
--platform=PLATFORM                                    platform name where Roc will run, supported values: empty (detect from host), 'linux', 'darwin', 'android'
--compiler=COMPILER                                    compiler name and optional version, e.g. 'gcc-4.9', supported names: empty (detect what available), 'gcc', 'clang'
--sanitizers=SANITIZERS                                list of gcc/clang sanitizers, supported names: empty (no sanitizers), 'all', 'undefined', 'address'
--enable-debug                                         enable debug build for Roc
--enable-debug-3rdparty                                enable debug build for 3rdparty libraries
--enable-werror                                        treat warnings as errors
--enable-pulseaudio-modules                            enable building of pulseaudio modules
--disable-lib                                          disable libroc building
--disable-tools                                        disable tools building
--enable-tests                                         enable tests building and running (requires CppUTest)
--enable-benchmarks                                    enable bechmarks building and running (requires Google Benchmark)
--enable-examples                                      enable examples building
--enable-doxygen                                       enable Doxygen documentation generation
--enable-sphinx                                        enable Sphinx documentation generation
--disable-soversion                                    don't write version into the shared library and don't create version symlinks
--disable-openfec                                      disable OpenFEC support required for FEC codes
--disable-sox                                          disable SoX support in tools
--disable-libunwind                                    disable libunwind support required for printing backtrace
--disable-pulseaudio                                   disable PulseAudio support in tools
--with-pulseaudio=WITH_PULSEAUDIO                      path to the PulseAudio source directory used when building PulseAudio modules
--with-pulseaudio-build-dir=WITH_PULSEAUDIO_BUILD_DIR  path to the PulseAudio build directory used when building PulseAudio modules (needed in case you build PulseAudio out of source; if empty, the build directory is assumed to be the same as the source directory)
--with-openfec-includes=WITH_OPENFEC_INCLUDES          path to the directory with OpenFEC headers (it should contain lib_common and lib_stable subdirectories)
--with-includes=WITH_INCLUDES                          additional include directory, may be used multiple times
--with-libraries=WITH_LIBRARIES                        additional library directory, may be used multiple times
--build-3rdparty=BUILD_3RDPARTY                        download and build specified 3rdparty libraries, pass a comma-separated list of library names and optional versions, e.g. 'uv:1.4.2,openfec'
--override-targets=OVERRIDE_TARGETS                    override targets to use, pass a comma-separated list of target names, e.g. 'glibc,stdio,posix,libuv,openfec,...'

Variables
=========

- CPPFLAGS
- CXXFLAGS
- CFLAGS
- LDFLAGS
- STRIPFLAGS
- CXX
- CC
- CXXLD
- CCLD or LD
- AR
- RANLIB
- STRIP
- RAGEL
- GENGETOPT
- DOXYGEN
- SPHINX_BUILD
- BREATHE_APIDOC
- PKG_CONFIG
- PKG_CONFIG_PATH
- CONFIG_GUESS
- CLANG_FORMAT

Targets
=======

`omitted`
    build everything

``test``
    build everything and run tests

``bench``
    build everything and run benchmarks

``clean``
    full clean, including build results, downloaded third-parties, generated documentation, and scons database

``cleanbuild``
    remove only build results

``cleandocs``
    remove only generated documentation

``install``
    install build results into the system

``uninstall``
    remove build results from the system

``fmt``
    format source code (requires clang-format)

``docs``
    build website (includes ``doxygen`` and ``sphinx`` targets)

``doxygen``
    build doxygen documentation (requires doxygen and graphviz)

``sphinx``
    build sphinx documentation (requires doxygen, sphinx-build, and breathe-apidoc)

``{module}``
    build specific module, e.g. ``roc_pipeline``

``test/{module}``
    run tests for specific module, e.g. ``test/roc_pipeline``

``bench/{module}``
    run benchmarks for specific module, e.g. ``bench/roc_pipeline``
