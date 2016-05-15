Howto
-----

**Ubuntu 14.04 and later, Debian Jessie**

    # for Roc
    $ apt-get install g++ pkg-config scons gengetopt libsox-dev

    # for 3rd-parties
    $ apt-get install libtool autoconf automake make cmake

    # build and test
    $ scons --with-3rdparty=uv,openfec,cpputest test

**Ubuntu 15.10 and later**

    # for Roc
    $ apt-get install g++ pkg-config scons gengetopt libuv1-dev libsox-dev libcpputest-dev

    # for 3rd-parties
    $ apt-get install make cmake

    # build and test
    $ scons --with-3rdparty=openfec test

**Fedora 22 and later**

    # for Roc
    $ dnf install gcc-c++ pkgconfig scons gengetopt libuv-devel sox-devel

    # for 3rd-parties
    $ dnf install make cmake

    # build and test
    $ scons --with-3rdparty=openfec,cpputest test

**Centos 7 and later**

    # for developer packages
    $ yum install epel-release

    # for Roc
    $ yum install gcc-c++ pkgconfig scons gengetopt sox-devel

    # for 3rd-parties
    $ yum install libtool autoconf automake make cmake

    # build and test
    $ scons --with-3rdparty=uv,openfec,cpputest test

**Centos 5**

    # for developer packages
    $ yum install epel-release

    # for Roc
    $ yum install gcc-c++ pkgconfig scons python26

    # for 3rd-parties
    $ yum install libtool autoconf automake make cmake

    # build and test
    $ python26 /usr/bin/scons --with-3rdparty=uv,openfec,sox,gengetopt,cpputest test

Dependencies
------------

**Build-time:**
* GCC >= 4.1 or clang >= 3.4
* python 2.x >= 2.6
* scons
* pkg-config (optional, if you want installed dependencies to be auto-detected)
* gengetopt (optional, if you want to build tools)
* doxygen >= 1.6, graphviz (optional, if you want to build documentation)
* clang-tidy (optional, if you want to run static analyzer)
* clang-format >= 3.6 (optional, if you want to format code)
* libtool, autoconf, automake, make, cmake (optional, if you want to download and build external dependencies automatically)

**Runtime:**
* [libuv](http://libuv.org) >= 1.4
* [OpenFEC](http://openfec.org) (optional but recommended, for LDPC-Staircase FEC codec)
* [SoX](http://sox.sourceforge.net) >= 14.4.0 (optional, if you want to build tools)
* [CppUTest](http://cpputest.github.io) (optional, if you want to build tests)

**Notes:**
* if you use CppUTest-3.4 or earlier, build it with `--disable-memory-leak-detection` option

Building
--------

    $ scons --help
    $ scons <options> <arguments> <build-targets>

After building, tools and libraries are inside `bin/` directory.

**Options:**
* `-Q` - enable pretty output
* `-n` - dry run
* `-j N` - run N jobs in parallel
* `--enable-werror` - treat warnings as errors
* `--disable-tools` - don't build tools
* `--disable-tests` - don't build tests
* `--disable-doc` - don't build documentation
* `--disable-sanitizers` - don't use GCC/clang sanitizers
* `--with-openfec=yes|no` - enable/disable LDPC-Staircase codec from OpenFEC for (required for FEC support)
* `--with-sox=yes|no` - enable/disable audio I/O using SoX (required to build tools)
* `--with-3rdparty=uv,openfec,sox,gengetopt,cpputest` or `--with-3rdparty=all` -  automatically download and build specific or all external dependencies (static linking is used in this case)
* `--with-targets=posix,stdio,gnu,uv,openfec,sox` - manually select source code directories to be included in build

**Arguments**:
* `variant=debug|release` - select build variant
* `target=linux` - select target platform; currently only `linux` is supported
* `compiler=<compiler>[-<version>]` - select compiler and version to use; currently, only `gcc` and `clang` are supported
* `toolchain=<prefix>` - select toolchain prefix added to compiler, linker and other tools

**Build targets:**
* *omitted* - build everything
* `test` - build everything and run unit tests
* `doxygen` - build documentation
* `clean` - remove build results
* `fmt` - format source code (if [clang-format](http://clang.llvm.org/docs/ClangFormat.html) >= 3.6 is found in PATH, it's used with [.clang-format](.clang-format) config)
* `tidy` - run clang static analyzer (requires clang-tidy to be installed)
* `<module>` - build only specific module
* `test/<module>` - build and run tests only for specific module

**Environment variables:**
* `CC`, `CXX`, `LD`, `AR`, `RANLIB`, `GENGETOPT`, `DOXYGEN`, `PKG_CONFIG` - overwrite tools to use
* `CFLAGS`, `CXXFLAGS`, `LDFLAGS` - additional compiler/linker flags

Examples
--------

Automatically detect platform and compiler, and build everything:

    $ scons -Q

Also build and run tests:

    $ scons -Q test

Select compiler and build variant:

    $ scons -Q compiler=gcc variant=debug test

Select specific compiler version:

    $ scons -Q compiler=gcc-4.8
    $ scons -Q compiler=gcc-4.8.5

Build specific module (see [modules](src/modules/) directory):

    $ scons -Q roc_audio

And run tests:

    $ scons -Q test/roc_audio

Download and build libuv, OpenFEC and CppUTest, then build everything:

    $ scons -Q --with-3rdparty=uv,openfec,cpputest

Download and build all external dependencies, then build everything:

    $ scons -Q --with-3rdparty=all

Minimal build:

    $ scons -Q --disable-tools --disable-tests --disable-doc --with-openfec=no --with-sox=no
