Glossary
********

.. contents:: Table of contents:
   :local:
   :depth: 1

Participants
============

.. glossary::

*roc peer*
    A sender or receiver (see below).

*roc sender*
    An entity that converts an audio stream to packets and sends them via the network to one or multiple receivers. The input audio stream is usually read from a file or a soundcard.

*roc receiver*
    An entity that receives packets from multiple senders, converts them to audio streams and mixes the streams. The output audio stream is usually written to a file or a soundcard.

*roc session*
    An entity that corresponds to one sender connected to one receiver.

*roc sender session*
    Representation of roc session on sender side.

*roc receiver session*
    Representation of roc session on receiver side.

Audio
=====

.. glossary::

*audio stream*
    A continuous sequence of audio frames.

*audio frame*
    An array of audio samples. May contain samples for multiple channels packed in the interleaved format. For example, two-channel audio is packed as L R L R ...

*audio sample*
    A value of an audio signal in a particular moment of time. A sequence of audio samples represents how sound pressure changes during time.

*audio channel*
    An audio signal dimension. For example, stereo sound consists of two channels: left and right.

*sample rate*
    The number of audio samples per channel per second. For example, if an audio stream has two channels and 44100 sample rate, it means that a second contains 44100 samples for the left channel and 44100 samples for the right channel.

Packet
======

.. glossary::

*network packet*
    Network datagram. Contains UDP header, RTP header and/or FEC payload ID, and RTP payload or FEC symbols.

*audio packet*
    Network datagram containing a batch of audio samples. Consists of a header and payload. The header describes the stream identifier and the position of the packet in the stream. The payload contains packet samples, usually for multiple channels.

*source packet*
    Network datagram containing audio packet plus FEC payload ID provided by a FEC code.

*repair packet*
    Network datagram containing FEC symbols and FEC payload ID provided by a FEC code.

RTP
===

.. glossary::

*RTP*
    Real-time Transport Protocol, a network protocol used to deliver both audio streams and FEC symbols.

*medium*
    Media type carried by a stream, for example, audio or video.

*stream*
    A sequence of packets of a single medium, produced by a single source, and transmitted to a single destination. Every stream has its own timing and sequence number space. A stream usually has a single encoding, but it may be changed over time.

*session*
    An association among a set of participants. A participant may handle multiple sessions. A session may carry multiple streams. The session is identified by the destination transport address (receiver IP address and UDP port). The streams inside a session are identified by SSRC numbers.

*unicast session*
    A point-to-point session with two participants: one sender and one receiver.

*multicast session*
    A session with multiple participants: multiple senders and one receiver with a multicast address.

*ssrc*
    Synchronization source. Identifies a stream inside a session and should be unique only inside that session.

*seqnum*
    Sequence number. Identifies a packet inside a stream. All packets in a stream are numbered sequentially usually starting from a random number.

*timestamp*
    The sampling instant of the first octet in the payload. For audio encodings, it is usually the number of the first sample in the packet. In this case, all samples in the stream are numbered sequentially usually starting from a random number.

FEC
===

.. glossary::

*FEC*
    Forward Error Correction, a technique to reduce packet loss over unreliable or noisy communication channels. The central idea is that the sender adds some sort of redundancy to the data being sent which can be used by the receiver to restore lost packets.

*FECFRAME*
    FEC Framework. A set of specifications incorporating several FEC schemes into RTP.

*FEC scheme*
    A specification of the FEC code plus the corresponding format of the source and repair packets.

*FEC code*
    A specification of the FEC encoder used on the sender and FEC decoder used on the receiver.

*FEC encoder*
    Encodes a sequence of audio packets to a sequence of source and repair packets. The packet sequence is usually divided into blocks and the encoding is performed per-block.

*FEC decoder*
     Decodes a sequence of audio packets from a sequence of source and repair packets, with the possibility of restoring some lost packets. The packet sequence is usually divided into blocks and the decoding is performed per-block.

*FEC payload ID*
    FEC header or footer describing the position of a source or repair packet in a FEC block and the position of the FEC block inside the stream.

*FEC symbols*
    FEC redundant data generated by FEC encoder for a single repair packet.

*FEC block*
    A batch of source and repair packet encoded from a batch of several subsequent audio packets. Described by the sequence number of the block, the number of source packets in the block, and the number of repair packets in the block.
