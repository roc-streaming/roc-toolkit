Quick start
-----------

For quick start, see "Building and testing" section [on wiki](https://github.com/roc-project/roc/wiki#building-and-testing).

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
* libtool, autoconf, automake, patch, make, cmake (optional, if you want to download and build external dependencies automatically)

**Runtime:**
* [libuv](http://libuv.org) >= 1.4
* [OpenFEC](http://openfec.org) (optional but recommended, for LDPC-Staircase FEC codec)
* [SoX](http://sox.sourceforge.net) >= 14.4.0 (optional, if you want to build tools)
* [CppUTest](http://cpputest.github.io) (optional, if you want to build tests)

**Notes:**
* if you use CppUTest 3.4 or earlier, build it with `--disable-memory-leak-detection` option
* if you use OpenFEC, it's recommended to apply the following patches that are not accepted to upstream yet (they're automatically applied when `--with-3rdparty` option is used):
  * [openfec-1.4.2-32bit.patch](https://gist.github.com/gavv/4325c090fc21a3033988ad745c03bdff) (fix for 32-bit systems)
  * [openfec-1.4.2-trace.patch](https://gist.github.com/gavv/8a9d38841778319f9c5045fbb39e3668) (hide errors from stderr)

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
* `build={type}` - the type of system on which Roc is being compiled, e.g. `x86_64-pc-linux-gnu`, autodetected if empty
* `host={type}` - the type of system on which Roc will run, e.g. `arm-linux-gnueabihf`, set equal to `build` if empty; used as prefix for copiler, linker, and similar tools
* `platform={name}` - platform name on which Roc will run, e.g. `linux`, autodetected from `host` if empty
* `compiler={name}` - compiler name, e.g. `gcc` or `clang`
* `variant=${name}` - build variant, e.g. `release` (default) or `debug`
* `3rdparty_variant=${name}` - build variant for dependencies that are built using `--with-3rdparty`, e.g. `release` (default) or `debug` (however some dependencies don't support `debug` variant)

**Build targets:**
* *omitted* - build everything
* `test` - build everything and run unit tests
* `doxygen` - build documentation
* `clean` - remove build results
* `fmt` - format source code (if [clang-format](http://clang.llvm.org/docs/ClangFormat.html) >= 3.6 is found in PATH, it's used with [.clang-format](.clang-format) config)
* `tidy` - run clang static analyzer (requires clang-tidy to be installed)
* `{module}` - build only specific module
* `test/{module}` - build and run tests only for specific module

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

Cross-compile to armv7:

    $ scons -Q host=arm-linux-gnueabihf --with-3rdparty=uv,openfec,sox,cpputest
