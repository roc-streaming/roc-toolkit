TODO
====

Public API
----------

- [ ] Implement public API for `roc`
- [ ] Add documentation

Basic features
--------------

- [x] Client and server pipelines
- [x] Network I/O
- [x] Sound I/O
- [x] [LDPC FEC](https://en.wikipedia.org/wiki/Low-density_parity-check_code) using [OpenFEC](http://openfec.org/)
- [ ] Finish RTP support (*work in progress*)
- [ ] Minimal RTCP support
- [ ] Session negotiation (probably  [RTSP](https://en.wikipedia.org/wiki/Real_Time_Streaming_Protocol))

Advanced features
-----------------

- [ ] Retransmission (probably [RFC 4588](https://tools.ietf.org/html/rfc4588) and
[RFC 4585](https://tools.ietf.org/html/rfc4585))
- [ ] Dynamic latency adjustment
- [ ] Compression (probably [Opus](https://www.opus-codec.org/))
- [ ] Encryption (probably [SRTP](https://en.wikipedia.org/wiki/Secure_Real-time_Transport_Protocol))
- [ ] Take a look at [RaptorQ](https://tools.ietf.org/html/rfc6330) and [OpenRQ](https://github.com/openrq-team/OpenRQ)
- [ ] Take a look at various IoT protocols (e.g. [IoTivity](https://www.iotivity.org/))

Ports
-----

- [ ] Various Linux and *nix distros
- [ ] Notably, Raspberry Pi (Raspbian)
- [ ] Mac OS X
- [ ] Windows

Applications
------------

#### GStreamer

- [ ] GStreamer plugin (using `roc` as network transport)
- [ ] User app prototype using our GStreamer plugin to connect local source and remote sink

#### Pulseaudio 

- [ ] Pulseaudio module (using `roc` as network transport)
- [ ] Probably also support native pulseaudio protocol in `roc` (see below)

#### Relay

- [ ] Implement relay tool (a.k.a. retranslator) that forwards incoming traffic to next hop, adding features
  like FEC, retransmission, compression, etc. This may be useful to improve quality of RTP trafic from
  other clients or probably pulseaudio trafic.
