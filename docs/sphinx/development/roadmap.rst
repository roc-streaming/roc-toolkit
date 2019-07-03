Roadmap
*******

Basic features
==============

- ☑ Real-time streaming with guaranteed latency
- ☑ Session multiplexing
- ☑ Dynamic sample rate adjustment
- ☑ Static sample rate conversion
- ☑ Packet interleaving
- ☑ Stream breakage detection
- ☑ RTP support
- ☑ FECFRAME support (Reed-Solomon and LDPC-Staircase FEC codes based on `OpenFEC <http://openfec.org/>`_)
- ☐ RTCP support for receiver feedback
- ☐ Session negotiation
- ☐ Configurable network encodings
- ☐ Opus support
- ☐ Service discovery

Advanced features
=================

- ☐ Dynamic latency adjustment (requires RTCP)
- ☐ Dynamic adjustment of FEC block size (requires RTCP)
- ☐ Dynamic adjustment of FEC code rate (requires RTCP and XRs from `RFC 5725 <https://tools.ietf.org/html/rfc5725>`_)
- ☐ Dynamic audio bitrate adjustment
- ☐ Dynamic payload type switch
- ☐ QoS support
- ☐ Encryption (likely SRTP)
- ☐ Packet loss concealment
- ☐ Lip sync support
- ☐ Multicast support
- ☐ Multi-room support (requires some research)
- ☐ Surround sound
- ☐ Congestion control
- ☐ Retransmission (probably `RFC 4588 <https://tools.ietf.org/html/rfc4588>`_ and `RFC 4585 <https://tools.ietf.org/html/rfc4585>`_)
- ☐ More FEC codes (notably `RaptorQ <https://tools.ietf.org/html/rfc6330>`_, see `OpenRQ <https://github.com/openrq-team/OpenRQ>`_ and `orq <https://github.com/olanmatt/orq>`_)
- ☐ More audio encodings (lossless and lossy)
- ☐ Video support (requires some research)
- ☐ Hardware acceleration (maybe OpenMAX)

API
===

- ☑ Transport API (network sender and receiver)
- ☐ Audio API (recorder and player)
- ☐ Control API
- ☐ Discovery API (publisher and explorer)
- ☐ Packet API (packet encoder and decoder)

Tools
=====

- ☑ Sender
- ☑ Receiver
- ☑ Converter
- ☐ Relay

Integrations
============

- ☑ Roc-based transport for PulseAudio
- ☐ Roc-based service discovery for PulseAudio
- ☐ Bindings for various languages

Portability
===========

- ☑ Linux, including Raspberry Pi and clones
- ☐ Other \*nix systems
- ☑ macOS
- ☐ Android
- ☐ Windows
