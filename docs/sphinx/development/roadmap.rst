Roadmap
*******

Basic features
==============

- |:ballot_box_with_check:| Real-time streaming with guaranteed latency
- |:ballot_box_with_check:| Latency tuning and clock drift compensation driven by receiver
- |:ballot_box_with_check:| Latency tuning and clock drift compensation driven by sender
- |:ballot_box_with_check:| Packet loss recovery using FEC
- |:ballot_box_with_check:| Session multiplexing
- |:ballot_box_with_check:| Stream breakage detection
- |:ballot_box_with_check:| Multiple slots on sender and receiver
- |:ballot_box_with_check:| Configurable network encoding
- |:ballot_box_with_check:| Automatic channel and rate conversions
- |:ballot_box_with_check:| Automatic encoding conversions
- |:ballot_box_with_check:| Metrics reporting
- |:white_large_square:| State reporting and polling

Advanced features
=================

- |:white_large_square:| End-to-end latency estimation and tuning
- |:ballot_box_with_check:| Adaptive latency (requires RTCP)
- |:white_large_square:| Adaptive FEC block and rate (requires RTCP and XRs from `RFC 5725 <https://tools.ietf.org/html/rfc5725>`_)
- |:white_large_square:| Adaptive packet size
- |:white_large_square:| Adaptive bitrate (for lossy codecs)
- |:white_large_square:| Encryption support (SRTP and DTLS)
- |:white_large_square:| QoS support
- |:white_large_square:| Packet loss concealment (PLC)
- |:white_large_square:| Lip sync support
- |:ballot_box_with_check:| Multicast support
- |:white_large_square:| Multi-room support (synchronized playback)
- |:white_large_square:| Surround sound
- |:white_large_square:| Retransmission (probably `RFC 4588 <https://tools.ietf.org/html/rfc4588>`_ and `RFC 4585 <https://tools.ietf.org/html/rfc4585>`_)
- |:white_large_square:| Congestion control

Extensibility
=============

- |:ballot_box_with_check:| Provide custom logger
- |:white_large_square:| Provide custom allocator
- |:ballot_box_with_check:| Register custom payload types
- |:white_large_square:| Register custom codecs
- |:ballot_box_with_check:| Register custom PLC
- |:white_large_square:| Register custom effect
- |:white_large_square:| Register custom mixer

Protocols
=========

- |:ballot_box_with_check:| RTP/AVPF
- |:ballot_box_with_check:| FECFRAME (Reed-Solomon and LDPC-Staircase FEC codes based on `OpenFEC <http://openfec.org/>`_)
- |:ballot_box_with_check:| RTCP
- |:ballot_box_with_check:| RTCP XR (extended reports)
- |:white_large_square:| SDP
- |:white_large_square:| RTSP
- |:white_large_square:| SRTP
- |:white_large_square:| DTLS

Codecs
======

- |:ballot_box_with_check:| PCM
- |:white_large_square:| FLAC
- |:white_large_square:| Vorbis
- |:white_large_square:| Opus
- |:ballot_box_with_check:| Reed-Solomon FEC
- |:ballot_box_with_check:| LDPC-Staircase FEC
- |:white_large_square:| RaptorQ FEC
- |:white_large_square:| Convolutional FEC

Audio backends
==============

- |:ballot_box_with_check:| PulseAudio
- |:white_large_square:| PipeWire
- |:white_large_square:| ALSA
- |:white_large_square:| JACK
- |:white_large_square:| CoreAudio (macOS and iOS)
- |:white_large_square:| Oboe (Android)
- |:white_large_square:| WASAPI (Windows)
- |:ballot_box_with_check:| SoX (universal)
- |:white_large_square:| FFmpeg
- |:ballot_box_with_check:| libsndfile
- |:ballot_box_with_check:| WAV

API
===

- |:ballot_box_with_check:| Single-stream transport API (roc_sender / roc_receiver)
- |:white_large_square:| Multi-stream transport API (roc_transceiver)
- |:ballot_box_with_check:| Codec API (roc_sender_encoder / roc_receiver_decoder)
- |:white_large_square:| Discovery API (roc_publisher / roc_explorer)

Tools
=====

- |:ballot_box_with_check:| Sender (roc-send)
- |:ballot_box_with_check:| Receiver (roc-recv)
- |:ballot_box_with_check:| Loopback (roc-copy)
- |:white_large_square:| Daemon (rocd)

Integrations
============

- |:ballot_box_with_check:| Roc-based transport for PulseAudio (roc-pulse)
- |:ballot_box_with_check:| Roc-based transport for PipeWire (roc-source, roc-sink)
- |:ballot_box_with_check:| Virtual audio device for macOS (roc-vad)
- |:white_large_square:| Virtual audio device for Windows

Bindings
========

- |:ballot_box_with_check:| Java (roc-java)
- |:ballot_box_with_check:| Go (roc-go)
- |:white_large_square:| Rust

Portability
===========

- |:ballot_box_with_check:| Linux
- |:ballot_box_with_check:| GNU/Hurd
- |:ballot_box_with_check:| Other \*nix systems
- |:ballot_box_with_check:| macOS
- |:ballot_box_with_check:| Android
- |:white_large_square:| iOS
- |:white_large_square:| Windows
- |:white_large_square:| Zephyr
