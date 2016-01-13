TODO
====

Public API
----------

- [ ] Implement public API for `roc`
- [ ] Documentat API

Basic features
--------------

- [x] Client and server pipelines
- [x] Network I/O
- [x] Sound I/O
- [x] [LDPC FEC](https://en.wikipedia.org/wiki/Low-density_parity-check_code) using [OpenFEC](http://openfec.org/)
- [ ] Finish RTP support (*work in progress*)
- [ ] Minimal RTCP support
- [ ] Session negotiation (probably  [RTSP](https://en.wikipedia.org/wiki/Real_Time_Streaming_Protocol))
- [ ] Support more than two channels (should be easy)

Advanced features
-----------------

- [ ] Retransmission (probably [RFC 4588](https://tools.ietf.org/html/rfc4588) and
[RFC 4585](https://tools.ietf.org/html/rfc4585))
- [ ] Dynamic latency adjustment (requires RTCP)
- [ ] Dynamic adjustment of FEC code rate  (requires RTCP and [XRs](https://tools.ietf.org/html/rfc5725))
- [ ] Congestion control
- [ ] Compression ([lossless](https://en.wikipedia.org/wiki/Lossless_compression#Audio) and lossy,
probably [Opus](https://www.opus-codec.org/))
- [ ] Encryption (probably [SRTP](https://en.wikipedia.org/wiki/Secure_Real-time_Transport_Protocol))
- [ ] Take a look at [RaptorQ](https://tools.ietf.org/html/rfc6330) and [OpenRQ](https://github.com/openrq-team/OpenRQ)
- [ ] Take a look at various IoT protocols (e.g. [IoTivity](https://www.iotivity.org/))

Video support
-------------

- [ ] Implement video packet formats (RTP)
- [ ] Implement video processing elements (complementing `audio::Streamer`, `audio::Resampler`, etc.)
- [ ] Adopt `roc_pipeline` for video
- [ ] Add video input/output support

Ports
-----

- [x] Various Linux distros
- [ ] Notably, Raspberry Pi (Raspbian)
- [ ] Other *nix distros
- [ ] Mac OS X
- [ ] Windows

Applications
------------

#### Pulseaudio 

- [ ] Pulseaudio module (using `roc` as network transport)
- [ ] Probably also support native pulseaudio protocol in `roc` (see below)

#### Relay

- [ ] Implement relay tool (a.k.a. retranslator) that forwards incoming traffic to next hop, adding features
  like FEC, retransmission, compression, etc. This may be useful to improve quality of RTP trafic from
  other clients or probably pulseaudio trafic.
