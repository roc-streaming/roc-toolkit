Changelog
*********

.. contents:: Releases:
   :local:
   :depth: 1

Version 0.1.2 (in progress)
===========================

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

Documentation
-------------

* extend "Forward Erasure Correction codes" page
* add new pages: "Licensing", "Contacts", "Authors"
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
