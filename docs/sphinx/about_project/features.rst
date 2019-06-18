Features
********

High-level features
===================

.. seealso:: :doc:`/internals/fec`, :doc:`/internals/fe_resampler`

* streaming CD-quality audio using RTP

* maintaining pre-configured target latency

* restoring lost packets using Forward Erasure Correction codes

  * communicating redundant packets using FECFRAME
  * encoding and decoding using OpenFEC

* resampling

  * converting between the sender and receiver clock domains (on receiver)
  * converting between the network and input/output sample rates (on receiver and sender)
  * configurable resampler profiles for different CPU and quality requirements

* multiplexing

  * mixing simultaneous streams from multiple senders on the receiver
  * binding receiver to multiple ports with different protocols

* interleaving packets to increase chances of successful loss recovery

* session watchdog

  * detecting session shutdown and removing the session
  * detecting playback problems and restarting the session

Protocols and encodings
=======================

.. seealso:: :doc:`/internals/network_protocols`

* RTP

  * RTP AVP L16 encoding (lossless 44100Hz PCM 16-bit stereo)

* FECFRAME

  * Reed-Solomon (m=8) FEC scheme (lower latency, lower rates)
  * LDPC-Staircase FEC scheme (higher latency, higher rates)

API and tools
=============

.. seealso:: :doc:`/api`, :doc:`/running/command_line_tools`, :doc:`/running/pulseaudio_modules`

* transport API

  * roc_sender --- send audio stream to receiver
  * roc_receiver --- receive and mix audio streams from senders

* command-line tools

  * roc-send --- read audio stream from audio device or file and send to receiver
  * roc-recv --- receive and mix audio streams from senders and write to audio device or file
  * roc-conv --- run Roc resampler from command-line

* PulseAudio modules

  * module-roc-sink --- send audio stream written to the sink to receiver
  * module-roc-sink-input --- receive and mix audio streams from senders and write to the connected sink

Portability
===========

.. seealso:: :doc:`/portability/supported_platforms`, :doc:`/portability/tested_boards`

* supported operating systems

  * GNU/Linux
  * macOS
  * Android *(work in progress)*

* tested hardware architectures

  * x86_64
  * ARMv6
  * ARMv7 (Cortex-A 32-bit)
  * ARMv8 (Cortex-A 64-bit)
