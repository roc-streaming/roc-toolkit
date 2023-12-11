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
* libtool, intltool, autoconf, automake, make, cmake, meson, ninja (optional, install if you want Roc to download and build dependencies automatically)

Runtime dependencies
====================

* `libuv <https://libuv.org>`_ >= 1.5.0, recommended >= 1.35.0
* `libatomic_ops <https://github.com/ivmai/libatomic_ops/>`_ >= 7.6.0 (needed only on non-C11 compilers)
* `libunwind <https://www.nongnu.org/libunwind/>`_ >= 1.2.1 (optional, install if you want backtraces on a panic or a crash)
* `hedley <https://nemequ.github.io/hedley/>`_ >= 15 (single-header library, vendored in our repo)
* `dr_wav <https://github.com/mackron/dr_libs/blob/master/dr_wav.h/>`_ >= 0.13.14 (single-header library, vendored in our repo)
* `OpenFEC <http://openfec.org>`_ >= 1.4.2 (optional but recommended, install if you want FEC support)
* `OpenSSL <https://www.openssl.org/>`_ >= 1.1.1, recommended >= 3 (optional but recommended, install if you want DTLS and SRTP support)
* `SpeexDSP <https://github.com/xiph/speexdsp>`_ >= 1.2beta3 (optional but recommended, install if you want to employ fast Speex resampler)
* `SoX <https://sox.sourceforge.net>`_ >= 14.4.0 (optional, install if you want SoX backend in tools)
* `PulseAudio <https://www.freedesktop.org/wiki/Software/PulseAudio/>`_ >= 5.0 (optional, install if you want PulseAudio backend in tools or PulseAudio modules)

.. warning::

   libuv versions before 1.5.0 may have problems on 64-bit ARMs.

.. warning::

   If you want to install OpenFEC, it's highly recommended to use `our fork <https://github.com/roc-streaming/openfec>`_ or manually apply patches from it. The fork is automatically used when using ``--build-3rdparty=openfec`` option. It contains several bug-fixes (including leaks and segfaults) and improvements that are not available in the upstream.

.. note::

   SpeexDSP (libspeexdsp) below 1.2rc3 was part of Speex (libspeex) package.

Development dependencies
========================

* `CppUTest <http://cpputest.github.io>`_ >= 4.0 (optional, install if you want to build tests)
* `Google Benchmark <https://github.com/google/benchmark>`_ >= 1.5.5 (optional, install if you want to build benchmarks)
* `clang-format <https://clang.llvm.org/docs/ClangFormat.html>`_ >= 10 (optional, install if you want to format code)
* `doxygen <https://www.doxygen.nl/>`_ >= 1.6, `graphviz <https://graphviz.gitlab.io/>`_ (optional, install if you want to build doxygen or sphinx documentation)
* `sphinx <https://www.sphinx-doc.org/>`_, `breathe <https://github.com/michaeljones/breathe>`_ (optional, install if you want to build sphinx documentation)

.. warning::

   Different versions of clang-format may format the code differently, thus we restrict the range of allowed versions. If you use another version, CI checks may fail.
