Dependencies
************

Build dependencies
==================

* GCC >= 4.1 or Clang >= 3.4
* Python >= 2.6 or 3.x
* SCons
* Ragel
* gengetopt (optional, install if you want to build tools)
* pkg-config (optional, install if you want installed dependencies to be auto-detected)
* config.guess script (optional, used to auto-detect the system type; may be provided by autotools, automake, or libtool package)
* libtool, intltool, autoconf, automake, make, cmake (optional, install if you want Roc to download and build dependencies automatically)

Runtime dependencies
====================

* `libuv <http://libuv.org>`_ >= 1.5.0, recommended >= 1.35.0
* `libatomic_ops <https://github.com/ivmai/libatomic_ops/>`_ >= 7.6.0 (needed only on non-C11 compilers)
* `libunwind <https://www.nongnu.org/libunwind/>`_ >= 1.2.1 (optional, install if you want backtraces on a panic or a crash)
* `OpenFEC <http://openfec.org>`_ >= 1.4.2 (optional but recommended, install if you want to enable FEC support)
* `SpeexDSP <https://github.com/xiph/speexdsp>`_ >= 1.2beta3 (optional but recommended, install if you want to employ fast Speex resampler)
* `SoX <http://sox.sourceforge.net>`_ >= 14.4.0 (optional, install if you want SoX backend in tools)
* `PulseAudio <https://www.freedesktop.org/wiki/Software/PulseAudio/>`_ >= 5.0 (optional, install if you want PulseAudio backend in tools or PulseAudio modules)

.. warning::

   libuv versions before 1.5.0 may have problems on 64-bit ARMs.

.. warning::

   If you want to install OpenFEC, it's highly recommended to use `our fork <https://github.com/roc-project/openfec>`_ or manually apply patches from it. The fork is automatically used when using ``--build-3rdparty=openfec`` option. It contains several bug-fixes (including leaks and segfaults) and improvements that are not available in the upstream.

.. note::

   SpeexDSP (libspeexdsp) below 1.2rc3 was part of Speex (libspeex) package.

Development dependencies
========================

* `CppUTest <http://cpputest.github.io>`_ >= 3.4 (optional, install if you want to build tests)
* `Google Benchmark <https://github.com/google/benchmark>`_ >= 1.5.0 (optional, install if you want to build benchmarks)
* `clang-format <https://clang.llvm.org/docs/ClangFormat.html>`_ >= 3.8 (optional, install if you want to format code)
* `clang-tidy <http://clang.llvm.org/extra/clang-tidy/>`_ (optional, install if you want to run linter)
* `doxygen <http://www.stack.nl/~dimitri/doxygen/>`_ >= 1.6, `graphviz <https://graphviz.gitlab.io/>`_ (optional, install if you want to build doxygen or sphinx documentation)
* `sphinx <http://www.sphinx-doc.org/>`_, `breathe <https://github.com/michaeljones/breathe>`_ (optional, install if you want to build sphinx documentation)

.. warning::

   If you use CppUTest 3.4 or earlier, build it with ``--disable-memory-leak-detection`` option. This leak detection breaks our code. Note that we support building with clang sanitizers which include LeakSanitizer.
