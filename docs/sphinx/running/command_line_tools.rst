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

    $ roc-recv -vv -s rtp+rs8m::10001 -r rs8m::10002
    16:47:32.157 [dbg] roc_sndio: initializing pulseaudio backend
    16:47:32.157 [dbg] roc_sndio: initializing sox backend
    16:47:32.158 [dbg] roc_recv: pool: initializing: object_size=2064 poison=0
    16:47:32.158 [dbg] roc_recv: pool: initializing: object_size=2576 poison=0
    16:47:32.158 [dbg] roc_recv: pool: initializing: object_size=632 poison=0
    16:47:32.158 [dbg] roc_sndio: pulseaudio sink: opening sink: device=(null)
    16:47:32.159 [inf] roc_sndio: pulseaudio sink: opening stream: device=(null) n_channels=2 sample_rate=48000
    16:47:32.160 [dbg] roc_audio: mixer: initializing: frame_size=640
    16:47:32.160 [dbg] roc_sndio: pulseaudio sink: stream_latency=0
    16:47:32.160 [dbg] roc_netio: transceiver: starting event loop
    16:47:32.160 [inf] roc_netio: udp receiver: opened port 0.0.0.0:10001
    16:47:32.160 [inf] roc_pipeline: receiver: adding port rtp+rs8m:0.0.0.0:10001
    16:47:32.160 [inf] roc_netio: udp receiver: opened port 0.0.0.0:10002
    16:47:32.160 [inf] roc_pipeline: receiver: adding port rs8m:0.0.0.0:10002
    16:47:32.160 [dbg] roc_sndio: pump: starting main loop
    16:47:39.039 [inf] roc_pipeline: receiver: creating session: src_addr=127.0.0.1:35836 dst_addr=0.0.0.0:10001
    16:47:39.039 [dbg] roc_packet: delayed reader: initializing: delay=8820
    16:47:39.039 [dbg] roc_fec: of decoder: initializing: codec=rs m=8
    16:47:39.039 [dbg] roc_audio: depacketizer: initializing: n_channels=2
    16:47:39.039 [dbg] roc_audio: watchdog: initializing: max_blank_duration=96000 max_drops_duration=96000 drop_detection_window=14400
    16:47:39.040 [dbg] roc_audio: resampler: initializing: window_interp=128 window_size=32 frame_size=640 channels_num=2
    16:47:39.040 [dbg] roc_audio: latency monitor: initializing: target_latency=8820 in_rate=44100 out_rate=48000
    16:47:39.040 [dbg] roc_packet: router: detected new stream: source=587064425 flags=0x8u
    16:47:39.040 [dbg] roc_audio: depacketizer: ts=320 loss_ratio=0.00000
    16:47:39.158 [dbg] roc_audio: watchdog: status: bbbbbbbbbbbbbbbbbbbb
    16:47:39.178 [dbg] roc_packet: router: detected new stream: source=0 flags=0x10u
    16:47:39.237 [dbg] roc_packet: delayed reader: initial queue: delay=8820 queue=9270 packets=30
    16:47:39.237 [dbg] roc_packet: delayed reader: trimmed queue: delay=8820 queue=8961 packets=29
    16:47:39.237 [dbg] roc_fec: fec reader: update payload size: next_esi=0 cur_size=0 new_size=1248
    16:47:39.237 [dbg] roc_fec: fec reader: update source block size: cur_sblen=0 cur_rblen=0 new_sblen=20
    16:47:39.237 [dbg] roc_audio: depacketizer: got first packet: zero_samples=9600
    16:47:39.246 [dbg] roc_audio: latency monitor: latency=8950 target=8820 fe=1.00000 trim_fe=1.00000 adj_fe=0.91875
    16:47:39.305 [dbg] roc_audio: watchdog: status: bbbbbbbbbb..........
    16:47:39.374 [dbg] roc_fec: fec reader: repair queue: dropped=10
    16:47:39.374 [dbg] roc_fec: fec reader: got first packet in a block, start decoding: n_packets_before=19 sbn=30736
    16:47:39.374 [dbg] roc_fec: fec reader: update repair block size: cur_sblen=20 cur_rblen=0 new_rblen=235
    16:47:42.218 [dbg] roc_sndio: pulseaudio sink: stream_latency=2761
    16:47:44.253 [dbg] roc_audio: latency monitor: latency=8776 target=8820 fe=1.00000 trim_fe=1.00000 adj_fe=0.91875
    ...

Running sender
==============

See :doc:`roc-send manual page </manuals/roc_send>` for the full list of options and some examples.

Here is an example of starting the sender reads audio stream from a WAV file and sends it to the receiver on 127.0.0.1 (locahost) with two UDP ports 10001 (for source packets) and 10002 (for repair packets).

.. code::

    $ roc-send -vv -i file:./input.wav -s rtp+rs8m:127.0.0.1:10001 -r rs8m:127.0.0.1:10002
    16:47:39.030 [dbg] roc_sndio: initializing pulseaudio backend
    16:47:39.031 [dbg] roc_sndio: initializing sox backend
    16:47:39.031 [dbg] roc_send: pool: initializing: object_size=2064 poison=0
    16:47:39.031 [dbg] roc_send: pool: initializing: object_size=2576 poison=0
    16:47:39.031 [dbg] roc_send: pool: initializing: object_size=632 poison=0
    16:47:39.031 [inf] roc_sndio: sox source: opening: driver=(null) input=./input.wav
    16:47:39.031 [dbg] roc_sndio: sox: formats.c: detected file format type `wav'
    16:47:39.031 [inf] roc_sndio: sox source: in_bits=16 out_bits=32 in_rate=44100 out_rate=0 in_ch=2 out_ch=0 is_file=1
    16:47:39.031 [dbg] roc_netio: transceiver: starting event loop
    16:47:39.031 [inf] roc_netio: udp sender: opened port 0.0.0.0:35836
    16:47:39.031 [inf] roc_pipeline: sender: using remote source port rtp+rs8m:127.0.0.1:10001
    16:47:39.031 [inf] roc_pipeline: sender: using remote repair port rs8m:127.0.0.1:10002
    16:47:39.031 [dbg] roc_fec: of encoder: initializing: codec=rs m=8
    16:47:39.031 [dbg] roc_fec: fec writer: update block size: cur_sbl=0 cur_rbl=0 new_sbl=20 new_rbl=10
    16:47:39.031 [dbg] roc_audio: packetizer: initializing: n_channels=2 samples_per_packet=309
    16:47:39.031 [dbg] roc_sndio: pump: starting main loop
    16:47:39.031 [dbg] roc_packet: router: detected new stream: source=587064425 flags=0x8u
    16:47:39.169 [dbg] roc_packet: router: detected new stream: source=0 flags=0x10u
    ...
