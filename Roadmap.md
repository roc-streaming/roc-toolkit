Roadmap
=======

Public API
----------

- [ ] Simple API (sample sender and receiver) (*work in progress*)
- [ ] Advanced API (packet encoder and decoder)

Basic features
--------------

- [x] Network I/O
- [x] Sound I/O
- [x] Dynamic resampling
- [ ] RTP support (*work in progress*)
- [ ] FECFRAME support with Reed-Solomon and LDPC Staircase [FEC](https://en.wikipedia.org/wiki/Forward_error_correction) codes using [OpenFEC](http://openfec.org/)  (*work in progress*)
- [ ] RTCP support for receiver feedback
- [ ] Session negotiation, likely using RTSP/SDP
- [ ] Service discovery using SAP/SDP

Advanced features
-----------------

- [ ] Dynamic latency adjustment (requires RTCP)
- [ ] Dynamic adjustment of FEC code rate  (requires RTCP and [XRs](https://tools.ietf.org/html/rfc5725))
- [ ] Compression ([lossless](https://en.wikipedia.org/wiki/Lossless_compression#Audio) and lossy,
probably [Opus](https://www.opus-codec.org/))
- [ ] Encryption (probably [SRTP](https://en.wikipedia.org/wiki/Secure_Real-time_Transport_Protocol))
- [ ] Congestion control
- [ ] Other FEC codes, notably [RaptorQ](https://tools.ietf.org/html/rfc6330) (see [OpenRQ](https://github.com/openrq-team/OpenRQ))
- [ ] Retransmission (probably [RFC 4588](https://tools.ietf.org/html/rfc4588) and
[RFC 4585](https://tools.ietf.org/html/rfc4585))
- [ ] [WebRTC](https://en.wikipedia.org/wiki/WebRTC) support for compatibility with browsers
- [ ] [OpenMAX](https://en.wikipedia.org/wiki/OpenMAX) support for hardware acceleration
- [ ] Video support

Portability
-----------

- [x] Linux distros, including Raspberry Pi
- [ ] Other *nix distros
- [ ] Mac OS X (*work in progress*)
- [ ] Android
- [ ] Windows

Tools
-----

- [x] Sender and receiver
- [ ] Relay (a.k.a. retranslator) that forwards incoming traffic to next hop, adding features
  like FEC, retransmission, compression, etc. This may be useful to improve quality of RTP trafic from
  existing applications.

PulseAudio
----------

- [ ] Roc-based transport for PulseAudio (*work in progress*)
- [ ] Roc-based service discovery for PulseAudio

Bindings
--------

- [ ] Go
- [ ] JavaScript (browser, node.js)
