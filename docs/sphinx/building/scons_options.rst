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

- ``./build/3rdparty/<host>/rpath``
    automatically built 3rd-party shared libraries

- ``./docs/man``
    generated manual pages

- ``./docs/html``
    generated HTML documentation

Options
=======

-Q                                             compact colored output
-c                                             remove build results
-n                                             dry run
-j N, --jobs=N                                 allow N parallel jobs at once

  --prefix=PREFIX             installation prefix, '/usr' by default
  --bindir=BINDIR             path to the binary installation directory (where to install Roc command-line
                                tools), '<prefix>/bin' by default
  --libdir=LIBDIR             path to the library installation directory (where to install Roc library),
                                auto-detected by default
  --incdir=INCDIR             path to the headers installation directory (where to install Roc headers),
                                '<prefix>/include' by default
  --mandir=MANDIR             path to the manuals installation directory (where to install Roc manual
                                pages), '<prefix>/share/man/man1' by default
  --build=BUILD               system name where Roc is being compiled, e.g. 'x86_64-pc-linux-gnu',
                                auto-detected by default
  --host=HOST                 system name where Roc will run, e.g. 'arm-linux-gnueabihf', auto-detected by
                                default
  --platform=PLATFORM         platform name where Roc will run, supported values: empty (detect from host),
                                'linux', 'unix', 'darwin', 'android'
  --compiler=COMPILER         compiler name and optional version, e.g. 'gcc-4.9', supported names:
                                empty (detect what available), 'clang', 'gcc', 'cc'
  --compiler-launcher=COMPILER_LAUNCHER
                              optional launching tool for c and c++ compilers, e.g. 'ccache'
  --sanitizers=SANITIZERS     list of gcc/clang sanitizers, supported names: empty (no sanitizers),
                                'all', 'undefined', 'address'
  --enable-debug              enable debug build for Roc
  --enable-debug-3rdparty     enable debug build for 3rdparty libraries
  --enable-werror             treat warnings as errors
  --enable-static             enable building static library
  --disable-shared            disable building shared library
  --disable-tools             disable tools building
  --enable-tests              enable tests building and running (requires CppUTest)
  --enable-benchmarks         enable benchmarks building and running (requires Google Benchmark)
  --enable-examples           enable examples building
  --enable-doxygen            enable Doxygen documentation generation
  --enable-sphinx             enable Sphinx documentation generation
  --disable-soversion         don't write version into the shared library and don't create version symlinks
  --disable-openfec           disable OpenFEC support required for FEC codes
  --disable-speexdsp          disable SpeexDSP support for resampling
  --disable-sox               disable SoX support in tools
  --disable-sndfile           disable sndfile support in tools
  --disable-openssl           disable OpenSSL support required for DTLS and SRTP
  --disable-libunwind         disable libunwind support required for printing backtrace
  --disable-libuuid           disable libuuid support for reliable UUID generation
  --disable-alsa              disable ALSA support in tools
  --disable-pulseaudio        disable PulseAudio support in tools
  --with-openfec-includes=WITH_OPENFEC_INCLUDES
                              path to the directory with OpenFEC headers (it should contain lib_common and
                                lib_stable subdirectories)
  --with-includes=WITH_INCLUDES
                              additional include search path, may be used multiple times
  --with-libraries=WITH_LIBRARIES
                              additional library search path, may be used multiple times
  --macos-platform=MACOS_PLATFORM
                              macOS target platform, e.g. 10.12, (default is current OS version)
  --macos-arch=MACOS_ARCH     macOS target architecture(s), comma-separated list, supported values: 'all',
                                'x86_64', 'arm64' (default is current OS arch, pass multiple values or
                                'all' for universal binaries)
  --build-3rdparty=BUILD_3RDPARTY
                              download and build specified 3rdparty libraries, comma-separated list of
                                library names and optional versions, e.g. 'libuv:1.4.2,openfec'
  --override-targets=OVERRIDE_TARGETS
                              override targets to use, pass a comma-separated list of target names, e.g.
                                'pc,posix,posix_ext,gnu,libuv,openfec,...'

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
- OBJCOPY
- INSTALL_NAME_TOOL
- RAGEL
- GENGETOPT
- DOXYGEN
- SPHINX_BUILD
- BREATHE_APIDOC
- PKG_CONFIG
- PKG_CONFIG_PATH
- CONFIG_GUESS
- CLANG_FORMAT
- DESTDIR

Targets
=======

`omitted` or ``.``
    build everything (including documentation, see ``docs`` target)

``test``
    build everything, then run tests

``bench``
    build everything, then run benchmarks

``clean``
    full clean, including build results, downloaded third-parties, generated documentation, and scons database

``clean_build``
    remove only build results

``clean_docs``
    remove only generated documentation

``install``
    install build results into the system or ``DESTDIR``

``uninstall``
    remove build results from the system or ``DESTDIR``

``fmt``
    format source code (requires clang-format)

``docs``
    build documentation, includes ``doxygen`` target if enabled with ``--enable-doxygen`` and ``sphinx`` target if enabled with ``--enable-sphinx``

``doxygen``
    build doxygen documentation (requires doxygen and graphviz)

``sphinx``
    build sphinx documentation (requires doxygen, sphinx-build, and breathe-apidoc)

``doctest``
    run python doctests from scripts (useful when you're updating scripts)

``{module}``
    build specific module, e.g. ``roc_pipeline``

``test/{module}``
    run tests for specific module, e.g. ``test/roc_pipeline``

``bench/{module}``
    run benchmarks for specific module, e.g. ``bench/roc_pipeline``
