Packets and frames
******************

.. contents:: Table of contents:
   :local:
   :depth: 1

Packets
=======

`Packet <https://roc-streaming.org/toolkit/doxygen/classroc_1_1packet_1_1Packet.html>`_ class from ``roc_packet`` module represents incoming or outgoing packet.

Packet holds the following information:

* **data** - Byte slice (``core::Slice``) that references binary data of the whole packet. `core::Slice <https://roc-streaming.org/toolkit/doxygen/classroc_1_1core_1_1Slice.html>`_ holds s shared pointer to `core::Buffer <https://roc-streaming.org/toolkit/doxygen/classroc_1_1core_1_1Buffer.html>`_ plus offset and length of the region inside the buffer.

* **headers** - Parsed protocol-specific fields. For example, `UDP <https://roc-streaming.org/toolkit/doxygen/structroc_1_1packet_1_1UDP.html>`_ struct holds UDP ports, and `RTP <https://roc-streaming.org/toolkit/doxygen/structroc_1_1packet_1_1RTP.html>`_ struct holds RTP timestamp, seqnum, and other fields. What headers are present in packet depends on packet type.

* **flags** - Bitmask that defines what kind of packet is this, what headers are available, and what action were already done with the packet.

Packet lifecycle
================

Packet life cycle depends on whether we're inside sender or receiver pipeline.

Typical packet **lifecycle on sender**:

* Allocate packet from packet pool (abstracted by `packet factory <https://roc-streaming.org/toolkit/doxygen/classroc_1_1packet_1_1PacketFactory.html>`_).

.. raw:: html

   <span/>

* Allocate buffer from buffer pool (abstracted by `buffer factory <https://roc-streaming.org/toolkit/doxygen/classroc_1_1core_1_1BufferFactory.html>`_) and attach buffer to packet.

.. raw:: html

   <span/>

* If there are specific requirements for payload alignment, ask `packet composer <https://roc-streaming.org/toolkit/doxygen/classroc_1_1packet_1_1IComposer.html>`_ to **align** packet. Composer adjusts packet's buffer in a way so that payload inside buffer would have desired alignment.

.. raw:: html

   <span/>

* Ask `packet composer <https://roc-streaming.org/toolkit/doxygen/classroc_1_1packet_1_1IComposer.html>`_ to **prepare** packet. Composer resizes packet's buffer to be able to hold given payload size and all necessary headers. Composer also enables appropriate header structs in packet (e.g. ``RTP`` or ``FEC``) by setting appropriate packet flags.

.. raw:: html

   <span/>

* Pass packet to pipeline of chained `packet writers <https://roc-streaming.org/toolkit/doxygen/classroc_1_1packet_1_1IWriter.html>`_. As packet goes through the pipeline, pipeline components may populate packet with more data, i.e. set fields of packet's header structs and encode samples directly into packet's buffer.

.. raw:: html

   <span/>

* In the end of pipeline, ask `packet composer <https://roc-streaming.org/toolkit/doxygen/classroc_1_1packet_1_1IComposer.html>`_ to **compose** packet. Composer finishes filling of packet's buffer by encoding all fields from header structs into corresponding parts of the packet's buffer.

.. raw:: html

   <span/>

* After the packet is composed, its buffer may be sent over network.

.. raw:: html

   <span/>

* After packet is sent, packet and packet's buffer may be returned to pools.

Typical packet **lifecycle on receiver**:

* Allocate packet from packet pool (abstracted by `packet factory <https://roc-streaming.org/toolkit/doxygen/classroc_1_1packet_1_1PacketFactory.html>`_).

.. raw:: html

   <span/>

* Allocate buffer from buffer pool (abstracted by `buffer factory <https://roc-streaming.org/toolkit/doxygen/classroc_1_1core_1_1BufferFactory.html>`_) and attach buffer to packet.

.. raw:: html

   <span/>

* Fill packet's buffer with data retrieved from network.

.. raw:: html

   <span/>

* Ask `packet parser <https://roc-streaming.org/toolkit/doxygen/classroc_1_1packet_1_1IParser.html>`_ to **parse** packet's buffer. Parser enables appropriate header structs in packet (e.g. ``RTP`` or ``FEC``) by setting appropriate packet flags, and fills these structs with information decoded from packet's buffer.

.. raw:: html

   <span/>

* Pass packet to pipeline of chained `packet readers <https://roc-streaming.org/toolkit/doxygen/classroc_1_1packet_1_1IReader.html>`_. As packet goes through the pipeline, pipeline components may read fields of packet's header structs and decode samples directly from packet's buffer.

.. raw:: html

   <span/>

* After packet is not needed anymore, packet and packet's buffer may be returned to pools.

For further details, see :doc:`/internals/pipelines`.

Packet parsers and composers
============================

Packet `parser <https://roc-streaming.org/toolkit/doxygen/classroc_1_1packet_1_1IParser.html>`_ and `composer <https://roc-streaming.org/toolkit/doxygen/classroc_1_1packet_1_1IComposer.html>`_ are interfaces that have implementations for various protocols, e.g. RTP or FECFRAME.

Both parsers and composers can be **chained** to implement stacking of protocols. For example, depending on FEC scheme, FECFRAME may require adding a footer to source packets. When such FEC scheme is used, pipeline will create two chained parsers/composers: the first one for FECFRAME protocol, and the second, nested one, for RTP protocol.

The chaining support is based on `slices <https://roc-streaming.org/toolkit/doxygen/classroc_1_1core_1_1Slice.html>`_. Packet's data field contains a slice that refers to a part of a buffer. When chaining is employed, the upper parser/composer creates a sub-slice of packet's buffer which corresponds to the nested protocol, and passes that sub-slice to the nested parser/composer. This way parser or composer does not need to be aware of whether it's the upper one or nested one.

Slices are also used in composer for payload alignment. Some pipeline components may have specific requirements for payload, for example, OpenFEC codec requires payload to be 8-byte aligned. To achieve this, FEC composer may sub-slice initial packet's buffer to shift its beginning in a way that after adding all headers, payload becomes properly aligned.

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

Frames
======

`Frame <https://roc-streaming.org/toolkit/doxygen/classroc_1_1audio_1_1Frame.html>`_ class from ``roc_audio`` module represents input or output audio frame.

* **samples** - Pointer to audio samples array and its length.

* **flags** - Bitmask that defines what additional information about the frame.

Unlike packet, a frame does not hold ownership of the samples array (packet holds a shared pointer to buffer). Frames are typically short-living objects allocated on stack and existing only during single pipeline tick.

Frame lifecycle
===============

Frame life cycle depends on whether we're inside sender or receiver pipeline.

Typical frame **lifecycle on sender**:

* Allocate frame on stack and attach to some samples array.

.. raw:: html

   <span/>

* Pass frame to pipeline of chained `frame writers <https://roc-streaming.org/toolkit/doxygen/classroc_1_1audio_1_1IFrameWriter.html>`_, requesting to process samples from this frame. A writer may pass the same frame further, or may create a new frame based on the provided one.

.. raw:: html

   <span/>

* Eventually, `packetizer <https://roc-streaming.org/toolkit/doxygen/classroc_1_1audio_1_1Packetizer.html>`_ produces packet(s) based on the frame, and the remainder of the pipeline will pass packets.

Typical frame **lifecycle on receiver**:

* Allocate frame on stack and attach to some samples array.

.. raw:: html

   <span/>

* Pass frame to pipeline of chained `frame reader <https://roc-streaming.org/toolkit/doxygen/classroc_1_1audio_1_1IFrameReader.html>`_, requesting to fill this frame with samples. A reader may pass the same frame further, or may create a new frame, request subsequent reader(s) to fill it, and then fill the provided frame based on that.

.. raw:: html

   <span/>

* Eventually, the request reaches `depacketizer <https://roc-streaming.org/toolkit/doxygen/classroc_1_1audio_1_1Depacketizer.html>`_, which consumes packet(s) from incoming queue and decodes them into the frame.

For further details, see :doc:`/internals/pipelines`.
