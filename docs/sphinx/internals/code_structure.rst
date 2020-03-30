Code structure
**************

.. contents:: Table of contents:
   :local:
   :depth: 1

Overview
========

Roc consists of the following high-level components:

* **Modules**

  Internal C++ libraries. Other components are based on them. See `Doxygen documentation <https://roc-project.github.io/roc/doxygen/>`_ for details.

* **Library**

  Public C API (libroc). A stable (not yet) interface for running Roc network sender and receiver. See :doc:`/api` page.

* **Tools**

  Command-line tools. A command-line interface for running Roc network sender and receiver, plus performing sound I/O, plus setting some debugging options. See :doc:`/running/command_line_tools` page.

* **PulseAudio modules**

  PulseAudio integration. Implement Roc-based network connectivity for PulseAudio. See :doc:`/running/pulseaudio_modules` page.

.. image:: ../_images/components.png
    :align: center
    :alt: High-level components

Modules
=======

Essentially, a module is just a C++ library providing a set of related classes. Every module has its own namespace, include path, and a set of unit tests. Modules may be built separately.

See `Doxygen documentation <https://roc-project.github.io/roc/doxygen/>`_ for details.

Modules can be grouped into several layers:

* network I/O layer (roc_netio)

* processing layer (roc_pipeline), with two sublayers:

 * packet processing sublayer (roc_packet, roc_rtp, roc_fec)

 * stream processing sublayer (roc_audio)

* sound I/O layer (roc_sndio)

On the receiver, data is transferring from the network layer to the sound layer. Accordingly, on the sender, data is transferring from the sound layer to the network layer.

See :doc:`/internals/data_flow` page for details.

Here is the full list of available modules:

================= =================================
module            description
================= =================================
roc_core          General-purpose building blocks (containers, memory management, multithreading, etc)
roc_packet        Network packets and packet processing
roc_rtp           RTP support
roc_fec           FEC support
roc_audio         Audio frames and audio processing
roc_pipeline      High-level sender and receiver pipelines on top of other modules
roc_netio         Network I/O
roc_sndio         Sound I/O
================= =================================

.. _targets:

Targets
=======

Roc supports multiple platforms and compilers. The major part of the source code is platform-independent. However, there are also parts that depend on specific platform features or optional third-party libraries.

The platform-dependent code is isolated inside "target" directories. Every target directory corresponds to a feature enabled at compile time. When SCons builds the project, it determines target directories to use, depending on the target platform, available third-party libraries, and command-line options.

Every module can have its own target directories. Headers from enabled target directories are added to the include path, and source files from enabled target directories are added to the build.

Currently supported targets are:

=================== =================
target              description
=================== =================
target_posix        Enabled for a POSIX OS
target_posixtime    Enabled for a POSIX OS with time extensions
target_gcc          Enabled for a GCC-compatible compiler
target_glibc        Enabled for the GNU standard C library
target_bionic       Enabled for the Bionic standard C library
target_darwin       Enabled for macOS
target_stdio        Enabled if stdio is available in the standard library
target_libuv        Enabled if libuv is available
target_libunwind    Enabled if libunwind is available
target_openfec      Enabled if OpenFEC is available
target_sox          Enabled if SoX is available
target_nobacktrace  Enabled if no backtrace API is available
target_nodemangle   Enabled if no demangling API is available
=================== =================

Example directory structure employing targets:

::

    roc_core
    ├── target_posix
    │   └── roc_core
    │       ├── ...
    │       ├── fast_random.cpp
    │       └── fast_random.h
    ├── target_posixtime
    │   └── roc_core
    │       ├── ...
    │       ├── time.cpp
    │       └── time.h
    ├── target_darwin
    │   └── roc_core
    │       ├── ...
    │       ├── time.cpp
    │       └── time.h
    ├── ...
    ├── array.h
    └── list.h
