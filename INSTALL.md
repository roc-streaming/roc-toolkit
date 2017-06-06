Quick start
-----------

For a quick start, see "Building and testing" section [on wiki](https://github.com/roc-project/roc/wiki#building-and-testing).

Dependencies
------------

**Build-time:**
* GCC >= 4.1 or clang >= 3.4
* python 2.x >= 2.6
* scons
* pkg-config (optional, use if you want installed dependencies to be auto-detected)
* gengetopt (optional, use if you want to build tools)
* doxygen >= 1.6, graphviz (optional, use if you want to build internal documentation)
* clang-tidy (optional, use if you want to run static analyzer)
* clang-format >= 3.8 (optional, use if you want to format code)
* libtool, autoconf, automake, make, cmake (optional, use if you want to download and build external dependencies automatically)

**Runtime:**
* [libuv](http://libuv.org) >= 1.4
* [OpenFEC](http://openfec.org) (optional but recommended, use if you want FEC support)
* [SoX](http://sox.sourceforge.net) >= 14.4.0 (optional, use if you want to build tools)
* [CppUTest](http://cpputest.github.io) >= 3.4 (optional, use if you want to build tests)

**Notes:**
* If you use CppUTest 3.4 or earlier, build it with `--disable-memory-leak-detection` option. It's leak detection doesn't work for us. Instead, we support building with clang sanitizers which include LeakSanitizer as well.
* If you use OpenFEC, it's recommended to use [our fork](https://github.com/roc-project/openfec) or manually apply patches from it. It is automatically selected by `--with-3rdparty=openfec` option. The fork contains several bugfixes and minor improvements that are not available in the upstream yet.

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
* `--disable-api` - don't build API libraries
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
* `host={type}` - the type of system on which Roc will run, e.g. `arm-linux-gnueabihf`, set equal to `build` if empty; used as prefix for compiler, linker, and similar tools
* `platform={name}` - platform name on which Roc will run, e.g. `linux`, autodetected from `host` if empty
* `compiler={name}` - compiler name, e.g. `gcc` or `clang`, autodetected if empty
* `variant=${name}` - build variant, e.g. `release` (default) or `debug`
* `3rdparty_variant=${name}` - build variant for dependencies that are built using `--with-3rdparty`, e.g. `release` (default) or `debug` (however only certain some dependencies support `debug` variant)

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
