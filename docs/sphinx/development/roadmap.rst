Roadmap
*******

Basic features
==============

- ☑ Network I/O
- ☑ Sound I/O
- ☑ Dynamic resampling
- ☑ RTP support
- ☑ FECFRAME support with Reed-Solomon and LDPC Staircase FEC codes using `OpenFEC <http://openfec.org/>`_
- ☐ RTCP support for receiver feedback
- ☐ Service discovery using SAP/SDP
- ☐ Session negotiation, likely using RTSP/SDP

Advanced features
=================

- ☐ Dynamic latency adjustment (requires RTCP)
- ☐ Dynamic adjustment of FEC block size
- ☐ Dynamic adjustment of FEC code rate (requires RTCP and XRs from `RFC 5725 <https://tools.ietf.org/html/rfc5725>`_)
- ☐ Compression (lossless and lossy, at least `Opus <https://www.opus-codec.org/>`_)
- ☐ Encryption (likely SRTP)
- ☐ Congestion control
- ☐ Other FEC codes (notably `RaptorQ <https://tools.ietf.org/html/rfc6330>`_, see `OpenRQ <https://github.com/openrq-team/OpenRQ>`_ and `orq <https://github.com/olanmatt/orq>`_)
- ☐ Retransmission (probably `RFC 4588 <https://tools.ietf.org/html/rfc4588>`_ and `RFC 4585 <https://tools.ietf.org/html/rfc4585>`_)
- ☐ OpenMAX support (for hardware acceleration)
- ☐ Video support (requires some research)

Portability
===========

- ☑ Linux, including Raspberry Pi and clones
- ☐ Other \*nix systems
- ☑ Mac OS X
- ☐ Android
- ☐ Windows

API
===

- ☑ Basic API (audio stream sender and receiver)
- ☐ Advanced API (packet encoder and decoder)

Tools
=====

- ☑ Sender
- ☑ Receiver
- ☐ Relay (will allow to improve quality of service of existing applications without modifying them)

PulseAudio
==========

- ☑ Roc-based transport for PulseAudio
- ☐ Roc-based service discovery for PulseAudio

Bindings
========

- ☐ Go
