Code structure
**************

.. warning::

   This section is under construction.

Modules
=======

Roc has a modular structure. Source code is divided into several modules, and we try to keep inter-module dependencies minimal. Dependencies should be specifies explicitly in ``src/build-deps.json``. If some module use another module, and the dependency is not specified there, build will fail (well, it should).

Every module:

* has its own namespace;
* has its own source code directory and include path;
* has its own unit tests;
* has its own SCons target and may be built separately;
* is compiled into separate static library.

=================== =================================
module              Description
=================== =================================
``roc_config``	    Global compile-time configuration options.
roc_core	        Lightweight general-purpose utility classes and wrappers for platform-dependent features.
``roc_datagram``	Protocol-independent network layer datagram processing (e.g. UDP).
``roc_packet``	    Protocol-independent application layer packet processing (e.g. RTP).
``roc_fec``	        FEC codecs. Acts at packet layer.
``roc_audio``	    Audio stream processing.
``roc_rtp``	        Implementation of packets for RTP.
``roc_pipeline``	    Sender and receiver pipelines. Glues roc_datagram, roc_packet, roc_fec, and roc_audio together to convert audio stream to datagrams (sender) or datagrams to audio stream (receiver).
``roc_sndio``	    Sound I/O. Allows to read or write audio stream from/to file or device.
``roc_netio``	    Network I/O. Allows to send or receive datagrams.
=================== =================================

.. image:: ../../diagrams/modules.png
	:height: 500
	:width: 791 px
	:alt: Rocs modules layout

.. _targets:

Targets
=======

Roc is targeted for multiple platforms and compilers. The major part of source code is platform-independent, however there are also parts that depend on platform or optional third-party libraries.

To keep code base clean, all platform-dependent code is strictly isolated inside ``target_`` directories. Every such directory corresponds to a feature supported by target platform. When SCons builds the project, it determines target directories to use, depending on target platform and available third-party libraries.

Every module can have its own target directories. Headers from all target directories in use are added to include path, and source files are added to build.

================== =================
Target             Description
================== =================
``target_posix``   Enabled when building for POSIX systems.
``target_stdio``   Enabled when standard input/output is supported in libc.
``target_gnu``     Enabled when GNU-compatible compiler is in use (GCC or clang).
``target_uv``      Enabled when libuv is available.
``target_openfec`` Enabled when OpenFEC is available.
``target_sox``     Enabled when SoX is available.
================== =================

Several target directories may contain alternative implementations of the same classes or functions, compatible at the source level. For example, ``class SpinLock`` could have independent declarations and implementations inside ``target_posix`` and ``target_win32``.
