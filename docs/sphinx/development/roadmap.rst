Roadmap
*******

Basic features
==============

- ☑ Real-time streaming with guaranteed latency
- ☑ Session multiplexing
- ☑ Dynamic sample rate adjustment
- ☑ Static sample rate conversion
- ☑ Packet interleaving
- ☑ Breakage detection
- ☑ RTP support
- ☑ FECFRAME support with Reed-Solomon and LDPC-Staircase FEC codes based on `OpenFEC <http://openfec.org/>`_
- ☐ RTCP support for receiver feedback
- ☐ Service discovery using SAP/SDP
- ☐ Session negotiation, likely using RTSP/SDP

Advanced features
=================

- ☐ Dynamic payload type (requires session negotiation)
- ☐ Dynamic latency adjustment (requires RTCP)
- ☐ Dynamic adjustment of FEC block size (requires RTCP)
- ☐ Dynamic adjustment of FEC code rate (requires RTCP and XRs from `RFC 5725 <https://tools.ietf.org/html/rfc5725>`_)
- ☐ Dynamic audio bitrate adjustment
- ☐ Congestion control
- ☐ Retransmission (probably `RFC 4588 <https://tools.ietf.org/html/rfc4588>`_ and `RFC 4585 <https://tools.ietf.org/html/rfc4585>`_)
- ☐ More FEC codes (notably `RaptorQ <https://tools.ietf.org/html/rfc6330>`_, see `OpenRQ <https://github.com/openrq-team/OpenRQ>`_ and `orq <https://github.com/olanmatt/orq>`_)
- ☐ QoS support
- ☐ Packet loss concealment
- ☐ Compression (lossless and lossy, at least `Opus <https://www.opus-codec.org/>`_)
- ☐ Encryption (likely SRTP)
- ☐ Surround sound
- ☐ Multicast support
- ☐ Multi-room support (requires some research)
- ☐ Video support (requires some research)
- ☐ Hardware acceleration (maybe OpenMAX)

Portability
===========

- ☑ Linux, including Raspberry Pi and clones
- ☐ Other \*nix systems
- ☑ macOS
- ☐ Android
- ☐ Windows

API
===

- ☑ Primary API (network sender and receiver)
- ☐ Advanced API (packet encoder and decoder)

Tools
=====

- ☑ Sender
- ☑ Receiver
- ☑ Converter
- ☐ Relay (to improve quality of service of existing applications without modifying them)

PulseAudio
==========

- ☑ Roc-based transport for PulseAudio
- ☐ Roc-based service discovery for PulseAudio

Bindings
========

- ☐ Bindings for other languages (maybe)
- ☐ GStreamer modules (maybe)
