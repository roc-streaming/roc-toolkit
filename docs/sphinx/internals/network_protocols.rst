Network protocols
*****************

Roc needs several network protocols to deliver data between the sender and the receiver.

Roc doesn't invent new protocols. Instead, it relies on existing open standards to take advantage of their careful design and potential interoperability with other software. Roc implements all protocols by itself, for two reasons. First, not all protocols have a suitable open implementation. Second, the protocol support is tightly integrated into the Roc pipeline and it would be hard to employ a stand-alone implementation.

Although Roc is designed to support arbitrary protocols, so far all supported protocols are RTP-based. RTP by itself is very basic but also very extensible. To employ RTP, one needs to select an RTP profile and, if necessary, some RTP extensions. Roc employs RTP Audio/Video Profile which specifies how to stream audio and video using RTP. Roc also employs FEC Framework, which specifies how to incorporate various FEC schemes into RTP.

Note that employing FEC Framework increases the service quality but also breaks interoperability with RTP implementations that are not aware of this extension. If you need compatibility with such applications, you should disable FEC in Roc

Roc also employs various RTCP extensions to exchange non-media reports between sender and receiver. Usually this doesn't break interoperability with implementations that don't support extensions, however some features may not work with such implementations (e.g. sender-side latency tuning).

Here is the full list of the network protocols implemented by Roc:

.. list-table::
   :widths: 10 60 30
   :header-rows: 1

   * - **RFC**
     - **Name**
     - **Comment**

   * - `RFC 3550 <https://tools.ietf.org/html/rfc3550>`_
     - `RTP: A Transport Protocol for Real-Time Applications`
     - Basic RTP and RTCP

   * - `RFC 3551 <https://tools.ietf.org/html/rfc3551>`_
     - `RTP Profile for Audio and Video Conferences with Minimal Control`
     - RTP AVPF (Audio/Video Profile)

   * - `RFC 3611 <https://tools.ietf.org/html/rfc3611>`_
     - `RTP Control Protocol Extended Reports`
     - | RTCP XR
       | (supported blocks: DLRR, RRTR)

   * - `RFC 6776 <https://tools.ietf.org/html/rfc6776>`_
     - `Measurement Identity and Information Reporting Using a Source Description (SDES) Item and an RTCP Extended Report (XR) Block`
     - RTCP XR Measurement Information Block

   * - `RFC 6843 <https://tools.ietf.org/html/rfc6843>`_
     - `RTP Control Protocol (RTCP) Extended Report (XR) Block for Delay Metric Reporting`
     - RTCP XR Delay Metrics Block

   * - `RFC 6363 <https://tools.ietf.org/html/rfc6363>`_
     - `Forward Error Correction (FEC) Framework`
     - FECFRAME

   * - `RFC 6865 <https://tools.ietf.org/html/rfc6865>`_
     - `Simple Reed-Solomon Forward Error Correction (FEC) Scheme for FECFRAME`
     - Reed-Solomon FEC Scheme

   * - `RFC 6816 <https://tools.ietf.org/html/rfc6816>`_
     - `Simple Low-Density Parity Check (LDPC) Staircase Forward Error Correction (FEC) Scheme for FECFRAME`
     - LDPC-Staircase FEC Scheme
