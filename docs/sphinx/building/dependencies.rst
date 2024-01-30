Dependencies
************

Build dependencies
==================

.. list-table::

   * - **dependency**
     - **comment**

   * - gcc >= 4.1 or clang >= 3.4
     - required

   * - python >= 2.6
     - required

   * - scons
     - required

   * - ragel
     - required

   * - gengetopt
     - required, unless you disable building command-line tools

   * - pkg-config
     - optional, used for better detection of system libraries

   * - config.guess
     - | optional, used for better detection of system type
       | (may be provided by autotools, automake, or libtool)

   * - | libtool, intltool, autoconf, automake
       | make, cmake, meson, ninja
     - | optional, used to build dependencies automatically
       | using ``--build-3rdparty`` option

Vendored dependencies
=====================

.. list-table::

   * - **dependency**
     - **version**
     - **comment**

   * - `dr_wav <https://github.com/mackron/dr_libs/blob/master/dr_wav.h/>`_
     - 0.13.14
     - single-header library

   * - `hedley <https://nemequ.github.io/hedley/>`_
     - 15
     - single-header library

Runtime dependencies
====================

.. list-table::

   * - **dependency**
     - **version**
     - **comment**

   * - `libatomic_ops <https://github.com/ivmai/libatomic_ops/>`_
     - >= 7.6.0
     - only required on pre-C11 compilers

   * - `libunwind <https://www.nongnu.org/libunwind/>`_
     - >= 1.2.1
     - optional, used to print backtraces

   * - `libuv <https://libuv.org>`_
     - >= 1.5.0 (recommended >= 1.35.0)
     - required

   * - `OpenFEC <http://openfec.org>`_
     - >= 1.4.2 (recommended to use `our fork <https://github.com/roc-streaming/openfec>`_)
     - optional, used for FECFRAME support

   * - `OpenSSL <https://www.openssl.org/>`_
     - >= 1.1.1
     - optional, used for SRTP and DTLS support

   * - `PulseAudio <https://www.freedesktop.org/wiki/Software/PulseAudio/>`_
     - >= 5.0
     - optional, used for PulseAudio I/O

   * - `SoX <https://sox.sourceforge.net>`_
     - >= 14.4.0
     - optional, used for audio I/O

   * - `SpeexDSP <https://github.com/xiph/speexdsp>`_
     - >= 1.2beta3
     - optional, used for fast resampling

.. note::

   For OpenFEC, it's highly recommended to use `our fork <https://github.com/roc-streaming/openfec>`_ or manually apply patches from it. The fork is automatically used when using ``--build-3rdparty=openfec`` option. It contains several critical fixes that are not available in the upstream.

.. note::

   libuv versions before 1.5.0 may have problems on 64-bit ARMs.

.. note::

   SpeexDSP (libspeexdsp) below 1.2rc3 was part of Speex (libspeex) package.

Development dependencies
========================

.. list-table::

   * - **dependency**
     - **comment**

   * - `CppUTest <http://cpputest.github.io>`_ >= 4.0
     - needed to build tests

   * - `Google Benchmark <https://github.com/google/benchmark>`_ >= 1.5.5
     - needed to build benchmarks

   * - `clang-format <https://clang.llvm.org/docs/ClangFormat.html>`_ >= 10
     - needed to format code with ``scons fmt``

   * - `doxygen <https://www.doxygen.nl/>`_ >= 1.6, `graphviz <https://graphviz.gitlab.io/>`_
     - needed to build doxygen and sphinx documentation

   * - `sphinx <https://www.sphinx-doc.org/>`_ >= 5, `breathe <https://github.com/michaeljones/breathe>`_
     - needed to build sphinx documentation

.. note::

   Different versions of clang-format may format the code differently, thus we restrict the range of allowed versions. If you use another version, CI checks may fail.
