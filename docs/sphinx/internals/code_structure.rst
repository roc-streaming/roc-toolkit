Code structure
**************

.. contents:: Table of contents:
   :local:
   :depth: 1

Overview
========

`Roc Toolkit <https://github.com/roc-streaming/roc-toolkit>`_ consists of the following high-level components:

* **Internal modules**

  Internal C++ libraries, not part of public API. Basic building blocks for library and tools. See `Doxygen documentation <https://roc-streaming.org/toolkit/doxygen/>`_ for details.

* **C library**

  Public C API (libroc) for C and C++ applications. See :doc:`/api` page.

* **Command-line tools**

  Command-line tools for developers and advanced users. See :doc:`/tools/command_line_tools` page.

Besides toolkit, `Roc Streaming <https://github.com/roc-streaming>`_ project provides some software on top of it:

* **Language bindings**

  Library bindings for other programming languages. See :doc:`/api/bindings` page.

* **Sound server modules**

  Modules for OS-specific sound servers, like PulseAudio and PipeWire. See :doc:`/tools/sound_server_modules` page.

* **Applications**

  Desktop and mobile apps for end-users. See :doc:`/tools/applications` page.

.. image:: ../_images/code_structure.png
    :align: center
    :alt: Overview

Modules
=======

Essentially, a module is just a C++ library providing a set of related classes. Every module has its own namespace, include path, and a set of unit tests. Modules may be built separately.

See `Doxygen documentation <https://roc-streaming.org/toolkit/doxygen/>`_ for details.

Modules can be grouped into several layers, as shown on the diagram above:

* network I/O layer (roc_netio)

* processing layer (roc_pipeline), with two sub-layers:

 * packet processing sub-layer (roc_packet, roc_rtp, roc_rtcp, roc_fec, and others)

 * frame processing sub-layer (roc_audio)

* sound I/O layer (roc_sndio)

* control layer (roc_ctl)

* peer layer (roc_peer)

On receiver media flows from network I/O layer, through processing layer, to sound I/O layer. Accordingly, on sender media flows from sound I/O layer, through processing layer, to network I/O layer. On both receiver and sender, there is also control layer that handles various supportive tasks. Finally, peer layer is a top-level layer which glues everything together.

Here is the full list of available modules:

================= =================================
module            description
================= =================================
roc_core          General-purpose building blocks (containers, memory management, multithreading, etc)
roc_address       Network URIs and addresses
roc_packet        Network packets and packet processing
roc_rtp           RTP support
roc_rtcp          RTCP support
roc_fec           FEC support
roc_sdp           SDP support
roc_audio         Audio frames and audio processing
roc_pipeline      Pipeline loop that arranges packet and frame processors into a chain
roc_ctl           Control loop that handles signaling protocols and background tasks
roc_netio         Network I/O loop
roc_sndio         Sound I/O loop
roc_peer          Top-level module that glues everything together
================= =================================

.. _targets:

Targets
=======

Roc supports multiple platforms and compilers. The major part of the source code is platform-independent. However, there are also parts that depend on specific platform features or optional third-party libraries.

Such platform-dependent code is isolated inside "target" directories. Every target directory corresponds to platform or feature enabled at compile time. When SCons builds the project, it determines target directories to use, depending on the target platform, available third-party libraries, and command-line options.

Every module can have its own target directories. Headers from enabled target directories are added to the include path, and source files from enabled target directories are added to the build.

Currently supported targets are:

===================== ===============================================
target                description
===================== ===============================================
target_pc             Enabled for PC (like server, desktop, laptop)
target_posix          Enabled for a POSIX OS
target_posix_ext      Enabled for a POSIX OS with POSIX extensions
target_posix_pc       Enabled for a POSIX OS on PC
target_gnu            Enabled for GNU-like libc and compiler
target_darwin         Enabled for macOS
target_android        Enabled for Android
target_c11            Enabled for C11 compilers
target_libunwind      Enabled if libunwind is available
target_libatomic_ops  Enabled if libatomic_ops is available
target_libuv          Enabled if libuv is available
target_openfec        Enabled if OpenFEC is available
target_speexdsp       Enabled if SpeexDSP is available
target_sox            Enabled if SoX is available
target_pulseaudio     Enabled if PulseAudio is available
target_nobacktrace    Enabled if no backtrace API is available
target_nodemangle     Enabled if no demangling API is available
===================== ===============================================

Example directory structure employing targets:

::

    roc_core
    ├── target_posix
    │   └── roc_core
    │       ├── ...
    │       ├── mutex.cpp
    │       └── mutex.h
    ├── target_posix_ext
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
