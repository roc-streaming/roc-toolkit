Network protocols
*****************

Roc needs several network protocols to deliver data between the sender and the receiver.

Roc doesn't invent new protocols. Instead, it relies on existing open standards to take advantage of their careful design and potential interoperability with other software. Roc implements all protocols by itself, for two reasons. First, not all protocols have a suitable open implementation. Second, the protocol support is tightly integrated into the Roc pipeline and it would be hard to employ a stand-alone implementation.

Although Roc is designed to support arbitrary protocols, currently all supported protocols are RTP-based. RTP by itself is very basic but also very extensible. To employ RTP, one needs to select an RTP profile and, if necessary, some RTP extensions. Roc employs RTP Audio/Video Profile which specifies how to stream audio and video using RTP. Roc also employs FEC Framework, which specifies how to incorporate various FEC schemes into RTP.

Note that employing FEC Framework increases the service quality but also breaks interoperability with RTP implementations that are not aware of this extension. If you need compatibility with such applications, you should disable FEC in Roc.

Here is the full list of the network protocols implemented by Roc:

================================================= ================================ ============
RFC                                               name                             comment
================================================= ================================ ============
`RFC 3550 <https://tools.ietf.org/html/rfc3550>`_ RTP                              Real-time Transport Protocol
`RFC 3551 <https://tools.ietf.org/html/rfc3551>`_ RTP A/V Profile                  Audio and video profile for RTP
`RFC 6363 <https://tools.ietf.org/html/rfc6363>`_ FEC Framework                    A framework for adding various FEC schemes to RTP
`RFC 6865 <https://tools.ietf.org/html/rfc6865>`_ Simple Reed-Solomon FEC Scheme   FEC scheme for FECFRAME
`RFC 6816 <https://tools.ietf.org/html/rfc6816>`_ Simple LDPC-Staircase FEC Scheme FEC scheme for FECFRAME
================================================= ================================ ============
