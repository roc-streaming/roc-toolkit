Command-line tools
******************

.. contents:: Table of contents:
   :local:
   :depth: 1

Overview
========

Roc provides a set of command-line tools, the most important of which are roc-recv and roc-send.

The roc-send tool allows to grab an audio stream from a file or an audio device like a microphone or record the system output sound and send it to a remote receiver.

The roc-recv tool allows to receive audio streams from a single or multiple senders, mix all streams into one, and play on an audio device or write to an audio file.

Building and installing
=======================

See our :doc:`/building` page for details. In particular, the :doc:`/building/user_cookbook` page provides examples for popular distros.

After build, the tools can be found in the ``./bin/<host>`` directory, for example ``./bin/x86_64-pc-linux-gnu``. You can either use tools from this directory or install them into the system using "scons install" command described by the link above.

Running receiver
================

See :doc:`roc-recv manual page </manuals/roc_recv>` for the full list of options and some examples.

Here is an example of starting the receiver that listens on all interfaces on two UDP ports 10001 (for source packets) and 10002 (for repair packets) and plays the received audio on the default audio device.

.. code::

    $ roc-recv -vv -s rtp+rs8m://0.0.0.0:10001 -r rs8m://0.0.0.0:10002 -c rtcp://0.0.0.0:10003
    15:23:09.147 [6447] [dbg] roc_peer: pool: initializing: object_size=632 poison=0
    15:23:09.147 [6447] [dbg] roc_peer: pool: initializing: object_size=2064 poison=0
    15:23:09.147 [6447] [dbg] roc_peer: pool: initializing: object_size=4112 poison=0
    15:23:09.147 [6447] [dbg] roc_peer: context: initializing
    15:23:09.147 [6447] [dbg] roc_sndio: initializing pulseaudio backend
    15:23:09.147 [6447] [dbg] roc_sndio: initializing sox backend
    15:23:09.147 [6447] [dbg] roc_sndio: pulseaudio sink: opening sink: device=(null)
    15:23:09.147 [6448] [dbg] roc_netio: event loop: starting event loop
    15:23:09.148 [6449] [inf] roc_sndio: pulseaudio sink: opening stream: device=(null) n_channels=2 sample_rate=48000
    15:23:09.149 [6447] [dbg] roc_audio: mixer: initializing: frame_size=640
    15:23:09.149 [6447] [dbg] roc_peer: receiver peer: initializing
    15:23:09.149 [6447] [dbg] roc_pipeline: receiver source: adding slot
    15:23:09.149 [6449] [dbg] roc_sndio: pulseaudio sink: stream_latency=0
    15:23:09.149 [6447] [dbg] roc_pipeline: receiver slot: adding source endpoint rtp+rs8m
    15:23:09.149 [6448] [inf] roc_netio: udp receiver: opened port 0.0.0.0:10001
    15:23:09.149 [6447] [inf] roc_peer: receiver peer: bound source interface to rtp+rs8m://0.0.0.0:10001
    15:23:09.149 [6447] [dbg] roc_pipeline: receiver slot: adding repair endpoint rs8m
    15:23:09.149 [6448] [inf] roc_netio: udp receiver: opened port 0.0.0.0:10002
    15:23:09.149 [6447] [inf] roc_peer: receiver peer: bound repair interface to rs8m://0.0.0.0:10002
    15:23:09.149 [6447] [dbg] roc_sndio: pump: starting main loop
    15:23:13.898 [6447] [inf] roc_pipeline: session group: creating session: src_addr=127.0.0.1:44592 dst_addr=0.0.0.0:10001
    15:23:13.898 [6447] [dbg] roc_packet: delayed reader: initializing: delay=8820
    15:23:13.898 [6447] [dbg] roc_fec: openfec decoder: initializing: codec=rs m=8
    15:23:13.898 [6447] [dbg] roc_audio: depacketizer: initializing: n_channels=2
    15:23:13.898 [6447] [dbg] roc_audio: watchdog: initializing: max_blank_duration=96000 max_drops_duration=96000 drop_detection_window=14400
    15:23:13.898 [6447] [dbg] roc_audio: resampler: initializing: window_interp=128 window_size=32 frame_size=640 channels_num=2
    15:23:13.898 [6447] [dbg] roc_audio: latency monitor: initializing: target_latency=8820 in_rate=44100 out_rate=48000
    15:23:13.898 [6447] [dbg] roc_packet: router: detected new stream: source=994636018 flags=0x8u
    15:23:13.898 [6447] [dbg] roc_audio: depacketizer: ts=320 loss_ratio=0.00000
    15:23:14.016 [6447] [dbg] roc_audio: watchdog: status: bbbbbbbbbbbbbbbbbbbb
    15:23:14.036 [6447] [dbg] roc_packet: router: detected new stream: source=0 flags=0x10u
    15:23:14.115 [6447] [dbg] roc_packet: delayed reader: initial queue: delay=8820 queue=9579 packets=31
    15:23:14.115 [6447] [dbg] roc_packet: delayed reader: trimmed queue: delay=8820 queue=8961 packets=29
    15:23:14.115 [6447] [dbg] roc_fec: fec reader: update payload size: next_esi=0 cur_size=0 new_size=1248
    15:23:14.115 [6447] [dbg] roc_fec: fec reader: update source block size: cur_sblen=0 cur_rblen=0 new_sblen=20
    15:23:14.115 [6447] [dbg] roc_audio: depacketizer: got first packet: zero_samples=10240
    15:23:14.115 [6447] [dbg] roc_audio: latency monitor: latency=8950 target=8820 fe=1.00000 trim_fe=1.00000 adj_fe=0.91875
    15:23:14.155 [6447] [dbg] roc_audio: watchdog: status: bbbbbbbbbbbb........
    15:23:14.234 [6447] [dbg] roc_fec: fec reader: repair queue: dropped=10
    15:23:14.234 [6447] [dbg] roc_fec: fec reader: got first packet in a block, start decoding: n_packets_before=18 sbn=47461
    15:23:14.234 [6447] [dbg] roc_fec: fec reader: update repair block size: cur_sblen=20 cur_rblen=0 new_rblen=235
    15:23:19.128 [6447] [dbg] roc_audio: latency monitor: latency=8776 target=8820 fe=0.99999 trim_fe=0.99999 adj_fe=0.91874
    15:23:19.207 [6449] [dbg] roc_sndio: pulseaudio sink: stream_latency=26761
    ...

Running sender
==============

See :doc:`roc-send manual page </manuals/roc_send>` for the full list of options and some examples.

Here is an example of starting the sender reads audio stream from a WAV file and sends it to the receiver on 127.0.0.1 (locahost) with two UDP ports 10001 (for source packets) and 10002 (for repair packets).

.. code::

    $ roc-send -vv -i file:./input.wav -s rtp+rs8m://127.0.0.1:10001 -r rs8m://127.0.0.1:10002 -c rtcp://127.0.0.1:10003
    15:23:13.896 [6450] [dbg] roc_peer: pool: initializing: object_size=632 poison=0
    15:23:13.896 [6450] [dbg] roc_peer: pool: initializing: object_size=2064 poison=0
    15:23:13.896 [6450] [dbg] roc_peer: pool: initializing: object_size=4112 poison=0
    15:23:13.896 [6450] [dbg] roc_peer: context: initializing
    15:23:13.896 [6450] [dbg] roc_sndio: initializing pulseaudio backend
    15:23:13.896 [6450] [dbg] roc_sndio: initializing sox backend
    15:23:13.896 [6451] [dbg] roc_netio: event loop: starting event loop
    15:23:13.896 [6450] [inf] roc_sndio: sox source: opening: driver=(null) input=./input.wav
    15:23:13.896 [6450] [dbg] roc_sndio: sox: formats.c: detected file format type `wav'
    15:23:13.896 [6450] [inf] roc_sndio: sox source: in_bits=16 out_bits=32 in_rate=44100 out_rate=0 in_ch=2 out_ch=0 is_file=1
    15:23:13.896 [6450] [dbg] roc_peer: sender peer: initializing
    15:23:13.896 [6450] [inf] roc_pipeline: sender sink: adding slot
    15:23:13.897 [6451] [inf] roc_netio: udp sender: opened port 0.0.0.0:44592
    15:23:13.897 [6450] [inf] roc_peer: sender peer: bound source interface to 0.0.0.0:44592
    15:23:13.897 [6450] [dbg] roc_pipeline: sender slot: adding source endpoint rtp+rs8m
    15:23:13.897 [6450] [inf] roc_peer: sender peer: connected source interface to rtp+rs8m://127.0.0.1:10001
    15:23:13.897 [6450] [inf] roc_peer: sender peer: reusing source interface port for repair interface
    15:23:13.897 [6450] [dbg] roc_pipeline: sender slot: adding repair endpoint rs8m
    15:23:13.897 [6450] [dbg] roc_fec: openfec encoder: initializing: codec=rs m=8
    15:23:13.897 [6450] [dbg] roc_fec: fec writer: update block size: cur_sbl=0 cur_rbl=0 new_sbl=20 new_rbl=10
    15:23:13.897 [6450] [dbg] roc_audio: packetizer: initializing: n_channels=2 samples_per_packet=309
    15:23:13.897 [6450] [inf] roc_peer: sender peer: connected repair interface to rs8m://127.0.0.1:10002
    15:23:13.897 [6450] [dbg] roc_sndio: pump: starting main loop
    15:23:13.897 [6450] [dbg] roc_packet: router: detected new stream: source=994636018 flags=0x8u
    15:23:14.035 [6450] [dbg] roc_packet: router: detected new stream: source=0 flags=0x10u
    ...
