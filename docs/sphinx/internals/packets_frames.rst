Packets and frames
******************

.. contents:: Table of contents:
   :local:
   :depth: 1

Packets
=======

`Packet <https://roc-streaming.org/toolkit/doxygen/classroc_1_1packet_1_1Packet.html>`_ class from ``roc_packet`` module represents incoming or outgoing packet.

Packet holds the following information:

* **data** - Byte slice (``core::Slice``) that references binary data of the whole packet. `core::Slice <https://roc-streaming.org/toolkit/doxygen/classroc_1_1core_1_1Slice.html>`_ holds a shared pointer to `core::Buffer <https://roc-streaming.org/toolkit/doxygen/classroc_1_1core_1_1Buffer.html>`_ or `core::BufferView <https://roc-streaming.org/toolkit/doxygen/classroc_1_1core_1_1BufferView.html>`_ plus offset and length of the region inside the buffer.

* **headers** - Parsed protocol-specific fields. For example, `UDP <https://roc-streaming.org/toolkit/doxygen/structroc_1_1packet_1_1UDP.html>`_ struct holds UDP ports, and `RTP <https://roc-streaming.org/toolkit/doxygen/structroc_1_1packet_1_1RTP.html>`_ struct holds RTP timestamp, seqnum, and other fields. What headers are present in packet depends on packet type.

* **flags** - Bitmask that defines what kind of packet is this, what headers are available, and what action were already done with the packet.

Packet headers typically have some kind of sequence number and/or timestamp. Packets may be lost, duplicated, or reordered. Components that work with packets use sequence numbers and timestamps to determine position of each packet in the stream.

Packet payload may vary significantly depending on the protocol. It may be uncompressed audio, encoded audio chunks, redundancy data used for loss repair, structured control information, etc.

Packet types
============

There are three major types of packets:

* **source packets** -- Packets with encoded media data.

  Always present. When FEC is enabled, may also contain FEC-specific header or footer.

* **repair packets** -- Packets with encoded redundancy data.

  Present only when FEC is enabled. Format depends on FEC scheme. Used to restore lost source (media) packets on receiver.

* **control packets** -- Out-of-band control messages.

  Present unless disabled by user. May be used for session management, congestion control, latency estimation, synchronization, etc.

Typical examples are RTP for source packets, FECFRAME for repair packets, and RTCP for control packets. See :doc:`/internals/network_protocols` page for the list of supported protocols.

For further details about FECFRAME and RTCP usage, see :doc:`/internals/fec` and :doc:`/internals/timestamps`.

Packet life cycle
=================

Packet life cycle depends on whether we're inside sender or receiver pipeline.

Typical packet **life cycle on sender**:

* Allocate packet and packet buffer from pools (abstracted by `packet factory <https://roc-streaming.org/toolkit/doxygen/classroc_1_1packet_1_1PacketFactory.html>`_). Attach buffer to packet.

\

* If there are specific requirements for payload alignment, ask `packet composer <https://roc-streaming.org/toolkit/doxygen/classroc_1_1packet_1_1IComposer.html>`_ to **align buffer**. Composer adjusts packet buffer in a way so that payload inside buffer would have desired alignment.

\

* Ask `packet composer <https://roc-streaming.org/toolkit/doxygen/classroc_1_1packet_1_1IComposer.html>`_ to **prepare packet**. Composer resizes packet buffer to be able to hold given payload size and all necessary headers. Composer also enables appropriate header structs in packet (e.g. ``RTP`` or ``FEC``) by setting appropriate packet flags.

\

* Pass packet to pipeline of chained `packet writers <https://roc-streaming.org/toolkit/doxygen/classroc_1_1packet_1_1IWriter.html>`_. As packet goes through the pipeline, pipeline components may incrementally **populate packet** with data, i.e. set fields of packet's header structs and encode samples into packet buffer.

\

* In the end of pipeline, ask `packet composer <https://roc-streaming.org/toolkit/doxygen/classroc_1_1packet_1_1IComposer.html>`_ to **compose packet**. Composer finishes filling of packet buffer by encoding all fields from header structs into corresponding parts of the packet buffer.

\

* After the packet is composed, **send packet** over network.

\

* After packet is sent, return packet and packet buffer to their pools.

Typical packet **life cycle on receiver**:

* Allocate packet and packet buffer from pools (abstracted by `packet factory <https://roc-streaming.org/toolkit/doxygen/classroc_1_1packet_1_1PacketFactory.html>`_). Attach buffer to packet.

\

* **Fill buffer** with data retrieved from network.

\

* Ask `packet parser <https://roc-streaming.org/toolkit/doxygen/classroc_1_1packet_1_1IParser.html>`_ to **parse packet**. Parser enables appropriate header structs in packet (e.g. ``RTP`` or ``FEC``) by setting appropriate packet flags, and fills these structs with information decoded from packet buffer.

\

* Pass packet to pipeline of chained `packet readers <https://roc-streaming.org/toolkit/doxygen/classroc_1_1packet_1_1IReader.html>`_. As packet goes through the pipeline, pipeline components may **process packet** and read parsed header fields or decode samples from packet buffer.

\

* After packet is not needed anymore, return packet and packet buffer to their pools.

For further details, see :doc:`/internals/pipelines`.

Packet ownership
================

Packet and packet buffer are both reference-countable objects. Packet factories, writers, and readers pass packets using shared pointers. Packet itself holds a shared pointer to its buffer. Readers and writers follow simple rules:

* When packet is written to packet writer, the **right to modify** packet or packet buffer is **passed from caller to writer**. The caller may retain a reference to the packet if needed, but should assume that writer may modify packet immediately or later.

\

* When packet is fetched (``ModeFetch``) or peeked (``ModePeek``) from packet reader, the **right to modify** packet or packet buffer is **passed from reader to caller**. Reader may retain a reference to the packet if needed, but should assume that the caller may modify packet.

Packet read mode (fetch vs peek)
================================

All `packet readers <https://roc-streaming.org/toolkit/doxygen/classroc_1_1packet_1_1IReader.html>`_ support two reading modes:

* ``ModeFetch`` -- get next available packet, remove it from queue, and return it
* ``ModePeek`` -- try to return next available packet, but don't remove it from queue

``ModeFetch`` is the "normal" mode, used when we need to get next packet and move stream forward.

``ModePeek`` implements a kind of a look-ahead. It is used when we want to inspect next packet before deciding whether to fetch it from reader. Fetching is an irreversible action, as it moves the read pointer forward, and sometimes we may want to avoid it depending on what packet is next.

Here is an example when ``ModePeek`` is useful:

* Imagine we're reading packets from FEC reader and there are 10 packets per FEC block. We've read 7th packet in current block, and now it's time to play 8th packet. But, packets 8, 9, 10 were delayed by network and weren't repaired, and 1st packet of the next block already arrived.

\

* If we perform a regular fetch now (``ModeFetch``), FEC reader would move pointer to next available packet, i.e. 1st packet of the next block. After switching to next block, it looses the possibility to repair 9th and 10th packets from previous block even if more packets arrive by the time they're needed.

\

* In contrast, if we perform a peek (``ModePeek``) and see that the next available packet is not needed right now, we can skip fetch until next read. We still have to insert gap in place of 8th packet, as it's already time to play it. However, since we haven't switched to the next block, we still have a chance that 9th and 10th packets will arrive or repaired by the time when we need to play them.

It's not guaranteed that ``ModePeek`` always can see next packet. Depending on implementation and current state, packet reader may not be able to access next packet without moving stream position forward. In such cases, ``ModeFetch`` would return a packet, but ``ModePeek`` returns ``StatusDrain``.

Packet status codes
===================

Packet read and write operations return `status codes <https://roc-streaming.org/toolkit/doxygen/namespaceroc_1_1status.html>`_:

* ``StatusOK``

  Packet was successfully read or written.

* ``StatusDrain``

  Packet reader returns it when there are no packets to read right now (but more can arrive later). When peek mode is used (``ModePeek``), packet reader may also return it when look-ahead is not possible without moving stream position forward (but it may be possible to read packet using fetch mode).

  Packet writer never returns this status.

* *other code*

  Any other status indicates pipeline failure and typically causes session termination.

.. note::

   Packet readers and writers never return ``StatusPart``, as it's not possible to read or write a part of a packet.

Packet parsers and composers
============================

Packet `parser <https://roc-streaming.org/toolkit/doxygen/classroc_1_1packet_1_1IParser.html>`_ and `composer <https://roc-streaming.org/toolkit/doxygen/classroc_1_1packet_1_1IComposer.html>`_ are interfaces that have implementations for various protocols, e.g. RTP or FECFRAME.

Both parsers and composers can be chained to implement stacking of protocols. For example, depending on FEC scheme, FECFRAME may require adding a footer to source packets. When such FEC scheme is used, pipeline will create two chained parsers/composers: the first one for FECFRAME protocol, and the second, nested one, for RTP protocol.

The chaining support is based on `slices <https://roc-streaming.org/toolkit/doxygen/classroc_1_1core_1_1Slice.html>`_. Packet's data field contains a slice that refers to a part of a buffer. When chaining is employed, the upper parser/composer creates a sub-slice of packet buffer which corresponds to the nested protocol, and passes that sub-slice to the nested parser/composer. This way parser or composer does not need to be aware of whether it's the upper one or nested one.

Slices are also used in composer for payload alignment. Some pipeline components may have specific requirements for payload, for example, OpenFEC codec requires payload to be 8-byte aligned. To achieve this, FEC composer may sub-slice initial packet buffer to shift its beginning in a way that after adding all headers, payload becomes properly aligned.

Frames
======

`Frame <https://roc-streaming.org/toolkit/doxygen/classroc_1_1audio_1_1Frame.html>`_ class from ``roc_audio`` module represents input or output audio frame.

Frame holds the following information:

* **buffer** - Byte slice (``core::Slice``) that references binary data of the frame. `core::Slice <https://roc-streaming.org/toolkit/doxygen/classroc_1_1core_1_1Slice.html>`_ holds s shared pointer to `core::Buffer <https://roc-streaming.org/toolkit/doxygen/classroc_1_1core_1_1Buffer.html>`_ or `core::BufferView <https://roc-streaming.org/toolkit/doxygen/classroc_1_1core_1_1BufferView.html>`_ plus offset and length of the region inside the buffer.

* **format** - Encoding identifier for the buffer contents.

* **flags** - Bitmask that defines additional characteristics of the frame, e.g. does it have samples decoded from packets or interpolated because of a packet loss.

* **duration** - How much samples does the frame contain (per audio channel).

* **capture timestamp** - Absolute time when the first sample of the frame was captured on sender (see :doc:`/internals/timestamps`).

Unlike packets, which may be lost or reordered, frames are always arranged into a **continuous stream**. Next frame always holds samples that are following immediately after the previous frame. If corresponding packet was lost, it is replaced with zeroized or interpolated samples, to keep stream continuous.

Samples in frame's buffer may have different encoding, depending on the format field of the frame. At different stages of the sender or receiver pipeline, frames may have different formats.

Frame formats
=============

There are three important categories of frames:

* **raw frames** - frame uses so-called "raw" format

  Raw format is a native-endian uncompressed PCM with 32-bit floats. Many pipeline elements can work only with frames in raw format (e.g. resampler). If network or sound card uses different format, a conversion is performed in the beginning on in the end of the pipeline.

* **pcm frames** - frame uses any PCM format

  Such frames still use PCM, but sample size and endian may be arbitrary (e.g. 24-bit big-endian unsigned integers). Some pipeline elements can work with arbitrary PCM frames, e.g. entry point in the beginning or in the end of the pipeline. For example, when a sound card may produce frames in non-raw PCM format, we still can do some simple operations on it, e.g. split frames into several sub-frames.

* **opaque frames**

  All non-PCM frames are considered opaque. We can't do much with such frames, except doing a verbatim copy or passing to decoder.

Frame life cycle
================

Frame life cycle depends on whether we're inside sender or receiver pipeline.

Typical frame **life cycle on sender**:

* Allocate frame and frame buffer from pools (abstracted by `frame factory <https://roc-streaming.org/toolkit/doxygen/classroc_1_1audio_1_1FrameFactory.html>`_). Attach buffer to frame.

\

* **Fill buffer** with the samples from user or sound card.

\

* Pass frame to pipeline of chained `frame writers <https://roc-streaming.org/toolkit/doxygen/classroc_1_1audio_1_1IFrameWriter.html>`_, requesting to **write samples** from this frame. A writer may pass the same frame further, or may create a new frame based on the provided one.

\

* Eventually, frame reaches `packetizer <https://roc-streaming.org/toolkit/doxygen/classroc_1_1audio_1_1Packetizer.html>`_, which **produces packets** based on the frame, and the remainder of the pipeline will pass packets.

Typical frame **life cycle on receiver**:

* Allocate frame from pool (abstracted by `frame factory <https://roc-streaming.org/toolkit/doxygen/classroc_1_1audio_1_1FrameFactory.html>`_).

\

* Optionally, allocate frame buffer and attach to the frame. If frame has pre-allocated buffer, frame reader is allowed, but not required, to use this buffer to write samples. If there is no pre-allocated buffer, or frame reader doesn't want to use it, it must allocate buffer by itself and attach it to frame.

\

* Pass frame to pipeline of chained `frame reader <https://roc-streaming.org/toolkit/doxygen/classroc_1_1audio_1_1IFrameReader.html>`_, requesting to **read samples** into the frame. A reader may pass the same frame further, or may create a new frame, request subsequent reader(s) to fill it, and then fill the provided frame based on that.

\

* Eventually, the request reaches `depacketizer <https://roc-streaming.org/toolkit/doxygen/classroc_1_1audio_1_1Depacketizer.html>`_, which **consumes packets** from incoming queue and decodes them into the frame.

For further details, see :doc:`/internals/pipelines`.

Frame ownership
===============

Frame and frame buffer are both reference-countable objects. Similar to packet, frame holds a shared pointer to its buffer. However, ownership rules for frames are different.

* When frame is written to frame writer, the **caller retains right to modify frame and buffer**. Frame writer is not allowed to modify frame or its buffer, and should assume that caller may modify them after the writer returns.

\

* When frame is requested from frame reader, again, the **caller retains right to modify frame and buffer**. Frame is allocated by caller and passed to frame reader, which should fill it with the result. Frame buffer may be allocated either by caller or by frame reader. In all cases, the caller owns both frame and buffer, and frame reader should assume that caller may modify them after the reader returns.

As mentioned above, when reading frame, the buffer may be either pre-allocated by caller, or allocated and returned by frame reader. It gives flexibility in memory management and allows to choose most efficient implementation depending on situation:

* In some cases, it is beneficial to pre-allocate frame and frame buffer and reuse them each time when we call frame reader. Due to pre-allocated buffer, read operation wouldn't require allocations.

\

* In other cases, it is more beneficial to allow frame reader to allocate buffer by itself. This is useful when frame reader already has its own buffer, and instead of copying data from it to caller's buffer, it can just attach (a slice of) existing buffer to the frame.

`Frame factory <https://roc-streaming.org/toolkit/doxygen/classroc_1_1audio_1_1FrameFactory.html>`_ provides convenient method ``reallocate_frame()`` that implements the first approach, suitable for most frame readers. It checks if the frame already has pre-allocated buffer large enough to fit the result. If not, it automatically allocates a new buffer and attaches it to the frame. After calling this call, frame is guaranteed to have a suitable buffer, no matter if it was pre-allocated or not.

Frame read mode (hard vs soft)
==============================

All `frame readers <https://roc-streaming.org/toolkit/doxygen/classroc_1_1audio_1_1IFrameReader.html>`_ support two reading modes:

* ``ModeHard`` -- read as much samples as possible, fill gaps caused by packet losses with zeros or interpolation
* ``ModeSoft`` -- read until next gap, but no further

``ModeHard`` is the "normal" mode, which is used to read frames when it's time to play them.

``ModeSoft`` is a mechanism for prefetching frames that are not needed right now, but will be needed soon. If next packets already arrived, it works the same as ``ModeHard``. However, if next packets are missing, it stops reading and doesn't move the stream forward.

Prefetching helps to counter occasional processing, scheduling, and I/O jitter without increasing latency:

* If we have time until next frame, we can perform a soft read to try decoding next frame in before, to reduce probability of an underrun.

\

* If soft read encounters a gap caused by packet loss, it stops. Hard read would instead fill the gap with zeros or interpolation. We don't want it because these packets still have a chance to arrive by the time when they should be played, and we shouldn't consider them lost until that.

When ``ModeSoft`` stops early, it returns either ``StatusPart`` (if it have read some samples before the gap), or ``StatusDrain`` (if it haven't read any samples at all). Note that ``StatusPart`` may be also caused by other reasons (see below).

Frame partial reads
===================

To read a frame, the caller provides `frame reader <https://roc-streaming.org/toolkit/doxygen/classroc_1_1audio_1_1IFrameReader.html>`_ with a frame and requested duration. Frame reader is allowed to return smaller duration than requested. This is called "partial read" and is indicated by ``StatusPart`` code.

Partial reads are used widely for various purposes:

* To fit the result into the maximum buffer size that can be allocated from frame buffer pool.

\

* To truncate the result by some internal chunk or packet boundary. This allows to simplify implementations of many readers, as they don't need to implement a loop that concatenates internal chunks into a single frame.

\

* To separate signal (decoded from packets) and gaps (caused by packet losses) into separate frames. This simplifies implementation of PLC (packet loss concealment), as it always can either forward or interpolate the whole frame.

\

* To implement soft reads (see above), by returning only data until next packet loss.

When the caller gets ``StatusPart`` (no matter if it used ``ModeHard`` or ``ModeSoft``), it is supposed to repeat the call in a loop until it gets ``StatusOK`` (frame is fully read) or ``StatusDrain`` (read stopped early, may happen only with ``ModeSoft``).

For simplicity, most pipeline elements just forward whatever status and frame they got and rely on upper levels to repeat the call if needed. The described loop is implemented in `mixer <https://roc-streaming.org/toolkit/doxygen/classroc_1_1audio_1_1Mixer.html>`_, which is the entry point to the readers pipeline.

Frame status codes
==================

Frame read and write operations return `status codes <https://roc-streaming.org/toolkit/doxygen/namespaceroc_1_1status.html>`_:

* ``StatusOK``

  Frame was successfully and fully read or written.

* ``StatusPart``

  Frame reader returns it when the frame was only partially read and has smaller duration than requested. This may happen during a soft read, or due to limitations or simplifications in implementation. For example, reader is allowed to truncate frame to fit maximum buffer size or chunk boundary.

  Frame writer never returns this status.

* ``StatusDrain``

  Frame reader returns it when there are no samples to read. This happens only for soft read when next packet is missing. It can't happen for hard read because missing packets are replaced with zeros or interpolation.

  Frame writer never returns this status.

* *other code*

  Any other status indicates pipeline failure and typically causes session termination.

Frame encoders and decoders
===========================

Frame `encoder <https://roc-streaming.org/toolkit/doxygen/classroc_1_1audio_1_1IFrameEncoder.html>`_ and `decoder <https://roc-streaming.org/toolkit/doxygen/classroc_1_1audio_1_1IFrameDecoder.html>`_ are interfaces that have implementations for various codecs, e.g. PCM or FLAC.

Frame encoder is used on sender in `packetizer <https://roc-streaming.org/toolkit/doxygen/classroc_1_1audio_1_1Packetizer.html>`_ to encode raw frame into opaque packet payload. Frame decoder is used on receiver in `depacketizer <https://roc-streaming.org/toolkit/doxygen/classroc_1_1audio_1_1Depacketizer.html>`_ to decode packet payload into raw frame.

Some codecs may implement extra features besides encoding and decoding, e.g. Opus codec is capable of restoring lost packets with a reduced quality, if the packet following the lost one is available.
