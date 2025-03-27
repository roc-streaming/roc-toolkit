Timestamps
**********

.. contents:: Table of contents:
   :local:
   :depth: 1

Types of timestamps
===================

:doc:`Packets and frames </internals/packets_frames>` have four major types of timestamps:

* STS - stream timestamp
* CTS - capture timestamp
* RTS - receive timestamp
* QTS - queue timestamp

**Stream timestamp (STS)** describes position of the first sample in packet or frame using abstract stream clock.

This clock corresponds to the sample source on sender and has sample rate of the stream. For example, if sender is 44100 Hz 2-channel stereo audio card, then STS is incremented by one each of two generated samples (left and right), and it happens 44100 times per second, according to the audio card clock.

Note that this clock is in a different domain compared to sample sink on receiver, or CPU on receiver, or even CPU on sender.

STS directly corresponds to the "timestamp" 32-bit field in RTP packets. STS starts from a random value (as required by RTP) and may periodically wrap.

**Capture timestamp (CTS)** describes the same event as STS, i.e. originating of the first sample in packet or frame, but using local Unix-time UTC clock, counting nanoseconds since Unix Epoch.

The clock for CTS always belongs to the local system, no matter if we're on sender or receiver:

* On sender, CTS of packet or frame is set to the system time when its first sample was captured.
* On receiver, CTS is set to an estimation of the same value, converted to receiver system clock, i.e. the system time *on receiver* when the first sample was captured *on sender*.

Unlike STS, this field does not directly correspond to any field inside RTP packet. Instead, sender and receiver exchange RTCP packets which help them to map STS to CTS, as described in the further sections.

**Receive timestamp (RTS)** is the time when the packet reached incoming network queue on receiver.

The clock for RTS is the same as for CTS: local Unix-time UTC clock, counting nanoseconds since Unix Epoch.

This timestamp is used only on receiver and only for packets.

**Queue timestamp (QTS)** is the time when the packet reached pipeline queue on receiver.

The clock for RTS is the same as for CTS: local Unix-time UTC clock, counting nanoseconds since Unix Epoch.

The difference between RTS and QTS is the time the packet spends in incoming queue until pipeline thread fetches it. QTS allows us to account additional jitter introduced by processing and thread-switch time.

This timestamp is used only on receiver and only for packets.

Use of timestamps
=================

Stream timestamps are used to position packets in the continuous stream of samples. When a packet arrives to receiver, its stream timestamp defines where exactly it will be inserted into receiver stream.

Capture and receive timestamps have the following usages:

* STS and CTS:

  * Estimate end-to-end latency between sender and receiver.

    To compute it, receiver needs to find the difference between the time when the frame was captured (i.e. capture timestamp) and the time when the frame is actually played (which receiver knows).

  * Maintain fixed end-to-end latency.

    If we have end-to-end latency metric on receiver, we can use it to drive latency tuning engine. Unlike NIQ latency (network queue size), end-to-end latency is very stable and allows more precise tuning and lower latency values.

  * Synchronize playback of receivers.

    For synchronous playback, it is enough to configure all receivers to maintain the same end-to-end latency. Since all of them will derive CTS from the same source (sender), playback will be automatically synchronous.

* RTS and QTS:

  * Estimate network jitter (using RTS) or network + processing + thread-switch jitter (using QTS).

    Receiver monitors these timestamps to determine jitter of the arriving packets. Then it may instruct latency tuner to keep latency above the jitter to prevent glitches caused by jitter.

Timestamp mapping
=================

RTP packet only has one timestamp, which in our codebase is called stream timestamp (STS).

It means that packets and frames can have capture timestamps (CTS) inside sender pipeline, but this information is erased when packets are sent via network. To restore capture timestamps on receiver, we use the help of RTCP protocol. RTCP allows to exchange mapping of NTP timestamp (similar to our CTS but in NTP time instead of Unix time) to RTP timestamp (synonym for our STS).

This is how it works:

* Sender maintains a mapping between STS and CTS, and updates this mapping each time a new packet is produced. Periodically sender generates an RTCP packet, which contains the latest mapping.

* Receiver maintains the same mapping, and updates this mapping whenever it receives an RTCP packet. Using this mapping, receiver is able to assign each packet a CTS based on its STS field.

This logic is implemented in `TimestampExtractor <https://roc-streaming.org/toolkit/doxygen/classroc_1_1rtp_1_1TimestampExtractor.html>`_ (on sender) and `TimestampInjector <https://roc-streaming.org/toolkit/doxygen/classroc_1_1rtp_1_1TimestampInjector.html>`_ (on receiver).

There are two subtleties here:

* First, sender and receiver system time is not necessarily synchronized.

  * If both sender and receiver support RTCP XR (`RFC 3611: RTCP Extended Reports <https://datatracker.ietf.org/doc/html/rfc3611>`_), which is true when both use Roc, then receiver will automatically compute offset between system clocks of sender and receiver based on RTT measurements (round trip time), and take it into account.

  * If RTCP XR is not supported, which may be true for some non-Roc senders, it's up to the user to keep system time of sender and receiver synchronized. Usually, this is achieved by configuring NTP daemon on all machines.

* Second, sender and receiver system time belongs to different clock domain, no matter if it's synchronized or not.

  * This problem is leveled by the periodic nature of RTCP reports. Each time an RTCP packet is sent, receiver obtains an up-to-date mapping of timestamps. Due to different clock domains, this mapping immediately starts drifting and accumulating error. However, the drift is slow, and RTCP reports are frequent, and the accumulated error never has time to become significant.

Timestamp forwarding
====================

Sender and receiver pipeline transfer packets and frames (see :doc:`/internals/pipelines`).

Sender pipeline starts with frames captured from audio card, which are then transformed to packets, which in the end reach outgoing network queue. Receiver pipeline, on the contrary, starts with packets from incoming network queue, which are then transformed to frames, which in the end reach audio card.

Here is the journey of capture timestamps through these pipelines:

* when a frame enters sender pipeline, it is assigned CTS based on current system time; if the frame was obtained from an audio card, latency of audio card (e.g. from PulseAudio or ALSA) is also taken into account

* frame is passed to the sender pipeline; all pipeline components properly forward CTS to derived frames and then to packets

* packet arrives to receiver and is assigned CTS based on RTCP mapping as described in previous section

* packet is passed to receiver pipeline; all pipeline components properly forward CTS to derived packets and then to frames

* when the frame leaves receiver pipeline, it has CTS, which is an estimation of CTS of the same frame on sender
