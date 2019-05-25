Changelog
*********

.. contents:: Releases:
   :local:
   :depth: 1

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
