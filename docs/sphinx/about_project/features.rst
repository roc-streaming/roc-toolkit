Features
********

High-level features
===================

.. seealso:: :doc:`/internals/fec`, :doc:`/internals/fe_resampler`

* streaming CD-quality audio using RTP

* maintaining pre-configured fixed or bounded latency

* restoring lost packets using Forward Erasure Correction codes

  * communicating redundant packets using FECFRAME
  * encoding and decoding using OpenFEC

* resampling

  * converting between the sender and receiver clock domains to compensate clock drift
  * converting between the network and input/output sample rates
  * configurable resampler backends and profiles for different CPU and quality requirements

* multiplexing

  * mixing simultaneous streams from multiple senders on the receiver
  * binding receiver to multiple ports with different protocols
  * connecting sender to multiple receivers with different protocols

* support for unicast, multicast, and broadcast

* interleaving packets to increase chances of successful loss recovery

* session watchdog

  * detecting session shutdown and removing the session
  * detecting playback problems and restarting the session

Protocols and encodings
=======================

.. seealso:: :doc:`/internals/network_protocols`

* RTP

  * RTP AVPF L16 encoding (lossless 44100Hz PCM 16-bit stereo or mono)

* RTCP

* RTCP XR

* FECFRAME

  * Reed-Solomon (m=8) FEC scheme (lower latency, lower rates)
  * LDPC-Staircase FEC scheme (higher latency, higher rates)

API and tools
=============

.. seealso:: :doc:`/api`, :doc:`/tools/command_line_tools`, :doc:`/tools/sound_server_modules`, :doc:`/tools/applications`

* transport API

  * roc_sender --- send audio stream to receiver
  * roc_receiver --- receive and mix audio streams from senders

* codec API

  * roc_sender_encoder --- encode audio stream into packets
  * roc_receiver_decoder --- decode audio stream from packets

* command-line tools

  * roc-send --- read audio stream from audio device or file and send to receiver
  * roc-recv --- receive and mix audio streams from senders and write to audio device or file
  * roc-copy --- copy audio between local audio devices or files

* PipeWire modules

  * roc-sink --- send audio stream written to the sink to receiver
  * roc-source --- receive and mix audio streams from senders and write to the connected apps

* PulseAudio modules

  * module-roc-sink --- send audio stream written to the sink to receiver
  * module-roc-sink-input --- receive and mix audio streams from senders and write to the connected sink

* macOS virtual device

  * roc-vad --- streaming audio device with CLI and gRPC interface

* mobile apps

  * Roc Droid -- Android app that connects your phone to other Roc senders and receivers

Bindings
========

.. seealso:: :doc:`/api/bindings`

* supported languages:

  * C (or C++)
  * Go
  * Java

Portability
===========

.. seealso:: :doc:`/portability/supported_platforms`, :doc:`/portability/tested_devices`

* supported operating systems

  * GNU/Linux
  * macOS
  * Android

* tested hardware architectures

  * x86_64
  * i686
  * ARM
  * MIPS
