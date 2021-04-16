Changelog
*********

.. contents:: Releases:
   :local:
   :depth: 1

Version 0.1.5 (Apr 5, 2020)
===========================

Building
--------

* correctly handle arguments in environment variables like CXX/CC/LD/etc (for Yocto Linux)
* correctly handle spaces in environment variables (for Yocto Linux)
* fix environment overrides checks
* fix building of the host tools when cross-compiling
* fix warnings on Clang 11
* fix sphinx invocation
* explicitly disable Orc when building PulseAudio using --build-3rdparty
* explicitly enable -pthread or -lpthread for libsndfile (for Manjaro Linux)
* user CMake instead of autotools when building libuv for Android using ``--build-3rdparty``
* switch to libuv 1.35.0 by default in ``--build-3rdparty``
* check for unknown names in ``--build-3rdparty``

Portability
-----------

* fix building on Manjaro Linux
* fix building on Yocto Linux
* add openSUSE to continuous integration and user cookbook
* drop Xcode 7.3 from continuous integration, add Xcode 11.3

Version 0.1.4 (Feb 6, 2020)
===========================

Internals
---------

* fix logging

Building
--------

* make ``/usr/local`` prefix default everywhere except Linux
* make default compiler consistent with CXX var
* fix handling of RAGEL, GENGETOPT, DOXYGEN, SPHINX_BUILD, and BREATHE_APIDOC vars
* fix SoX download URL (again)
* fix CPU count calculation

Documentation
-------------

* update PulseAudio version numbers in "User cookbook"
* update CONTRIBUTING and "Coding guidelines"
* update maintainers and contributors list

Version 0.1.3 (Oct 21, 2019)
============================

Tools
-----

* add ``--list-drivers`` option
* add git commit hash to version info

Internals
---------

* print backtrace on Linux and macOS using libunwind instead of glibc backtrace module
* print backtrace on Android using bionic backtrace module
* colored logging

Building
--------

* add libunwind optional dependency (enabled by default)
* add ragel required dependency
* rename "uv" to "libuv" in ``--build-3rdparty``
* don't hide symbols in debug builds
* strip symbols in release builds
* fix building on recent Python versions
* fix SoX download URL
* fix PulseAudio version parsing
* automatically apply memfd patch when building PulseAudio
* automatically fix libasound includes when building PulseAudio

Version 0.1.2 (Aug 14, 2019)
============================

Bugfixes
--------

* fix handling of inconsistent port protocols / FEC schemes
* fix IPv6 support
* fix incorrect usage of SO_REUSEADDR
* fix panic on bind error
* fix race in port removing code
* fix packet flushing mechanism
* fix backtrace printing on release builds

Internals
---------

* rework audio codecs interfaces (preparations for Opus and read-aheads support)
* minor refactoring in FEC support
* improve logging

Portability
-----------

* fix building on musl libc
* continuous integration for Alpine Linux

Building
--------

* allow to configure installation directories
* auto-detect system library directory and PulseAudio module directory

Documentation
-------------

* extend "Forward Erasure Correction codes" page
* add new pages: "Usage", "Publications", "Licensing", "Contacts", "Authors"
* replace "Guidelines" page with "Contribution Guidelines", "Coding guidelines", and "Version control"

Version 0.1.1 (Jun 18, 2019)
============================

Bugfixes
--------

* fix memory corruption in OpenFEC / LDPC-Staircase (fix available in our fork)
* fix false positives in stream breakage detection

Portability
-----------

* start working on Android port; Roc PulseAudio modules are now available in Termux unstable repo
* continuous integration for Android / arm64 (minimal build)
* docker image for aarch64-linux-android toolchain

Testing
-------

* fix resampler AWGN tests
* add travis job to run tests under valgrind

Building
--------

* fix multiple build issues on macOS
* fix multiple build issues with cross-compilation and Android build
* fix issues with building third-parties
* fix issues with compilation db generation
* set library soname/install_name and install proper symlinks
* improve configuration options
* improve system type detection and system tools search
* improve scripts portability
* better handling of build environment variables

Version 0.1.0 (May 28, 2019)
============================

Features
--------

* streaming CD-quality audio using RTP (PCM 16-bit stereo)
* maintaining pre-configured target latency
* restoring lost packets using FECFRAME with Reed-Solomon and LDPC-Staircase FEC schemes
* converting between the sender and receiver clock domains using resampler
* converting between the network and input/output sample rates
* configurable resampler profiles for different CPU and quality requirements
* mixing simultaneous streams from multiple senders on the receiver
* binding receiver to multiple ports with different protocols
* interleaving packets to increase the chances of successful loss recovery
* detecting and restarting broken streams

API
---

* initial version of transport API (roc_sender, roc_receiver)

Tools
-----

* initial version of command-line tools (roc-send, roc-recv, roc-conv)
* initial version of PulseAudio transport (module-roc-sink, module-roc-sink-input)

Portability
-----------

* GNU/Linux support
* macOS support
* continuous integration for Ubuntu, Debian, Fedora, CentOS, Arch Linux, macOS
* continuous integration for x86_64, ARMv6, ARMv7, ARMv8
* toolchain docker images for arm-bcm2708hardfp-linux-gnueabi, arm-linux-gnueabihf, aarch64-linux-gnu
* testing on Raspberry Pi 3 Model B, Raspberry Pi Zero W, Orange Pi Lite 2
