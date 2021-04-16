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
    [info] roc_sndio: initializing pulseaudio backend
    [info] roc_sndio: initializing sox backend
    [debug] roc_recv: pool: initializing: object_size=2064 poison=0
    [debug] roc_recv: pool: initializing: object_size=2576 poison=0
    [debug] roc_recv: pool: initializing: object_size=608 poison=0
    [debug] roc_sndio: pulseaudio sink: opening sink: device=(null)
    [debug] roc_sndio: pulseaudio sink: opening stream: device=(null) n_channels=2 sample_rate=48000
    [debug] roc_sndio: pulseaudio sink: stream_latency=0
    [info] roc_netio: udp receiver: opened port 0.0.0.0:10001
    [info] roc_netio: udp receiver: opened port 0.0.0.0:10002
    [debug] roc_sndio: pump: starting
    [debug] roc_netio: transceiver: starting event loop
    [info] roc_pipeline: receiver: creating session
    [debug] roc_packet: delayed reader: initializing: delay=8820
    [debug] roc_fec: of decoder: initializing Reed-Solomon decoder
    [debug] roc_audio: watchdog: initializing: max_blank_duration=96000 max_drops_duration=96000 drop_detection_window=14400
    [debug] roc_audio: resampler: initializing: window_interp=128 window_size=32 frame_size=640 channels_num=2
    [debug] roc_audio: resampler reader: initializing window
    [debug] roc_audio: latency monitor: initializing: target_latency=8820 in_rate=44100 out_rate=48000
    [debug] roc_packet: router: detected new stream: source=1995914915 flags=0x8u
    [debug] roc_audio: depacketizer: ts=320 loss_ratio=0.00000
    [debug] roc_audio: watchdog: status: bbbbbbbbbbbbbbbbbbbb
    [debug] roc_packet: router: detected new stream: source=0 flags=0x10u
    [debug] roc_packet: delayed reader: initial queue: delay=8820 queue=9270 packets=30
    [debug] roc_packet: delayed reader: trimmed queue: delay=8820 queue=8961 packets=29
    [debug] roc_fec: fec reader: update sblen, cur_sbl=0 new_sbl=20
    [debug] roc_audio: depacketizer: got first packet: zero_samples=9600
    [debug] roc_audio: latency monitor: latency=8950 target=8820 fe=1.00000 trim_fe=1.00000 adj_fe=0.91875
    [debug] roc_audio: watchdog: status: bbbbbbbbbb..........
    [debug] roc_fec: fec reader: repair queue: dropped=10
    [debug] roc_fec: fec reader: got first packet in a block, start decoding: n_packets_before=19 sn=49902 sbn=50206
    [debug] roc_audio: latency monitor: latency=8787 target=8820 fe=0.99999 trim_fe=0.99999 adj_fe=0.91874
    [debug] roc_sndio: pulseaudio sink: stream_latency=1792
    [debug] roc_audio: latency monitor: latency=8613 target=8820 fe=0.99981 trim_fe=0.99981 adj_fe=0.91858
    [debug] roc_audio: latency monitor: latency=8759 target=8820 fe=0.99980 trim_fe=0.99980 adj_fe=0.91856
    [debug] roc_sndio: pulseaudio sink: stream_latency=1642
    ...

Running sender
==============

See :doc:`roc-send manual page </manuals/roc_send>` for the full list of options and some examples.

Here is an example of starting the sender reads audio stream from a WAV file and sends it to the receiver on 127.0.0.1 (locahost) with two UDP ports 10001 (for source packets) and 10002 (for repair packets).

.. code::

    $ roc-send -vv -s rtp+rs8m:127.0.0.1:10001 -r rs8m:127.0.0.1:10002 -i ./file.wav
    [info] roc_sndio: initializing pulseaudio backend
    [info] roc_sndio: initializing sox backend
    [debug] roc_send: pool: initializing: object_size=2064 poison=0
    [debug] roc_send: pool: initializing: object_size=2576 poison=0
    [debug] roc_send: pool: initializing: object_size=608 poison=0
    [info] roc_sndio: sox source: opening: driver=(null) input=/home/victor/stash/loituma.wav
    [info] roc_sndio: [sox] formats.c: detected file format type `wav'
    [info] roc_sndio: [sox] wav.c: Searching for 66 6d 74 20
    [info] roc_sndio: [sox] wav.c: WAV Chunk fmt 
    [info] roc_sndio: [sox] wav.c: Searching for 64 61 74 61
    [info] roc_sndio: [sox] wav.c: WAV Chunk LIST
    [info] roc_sndio: [sox] wav.c: WAV Chunk data
    [info] roc_sndio: [sox] wav.c: Reading Wave file: Microsoft PCM format, 2 channels, 44100 samp/sec
    [info] roc_sndio: [sox] wav.c:         176400 byte/sec, 4 block align, 16 bits/samp, 28832256 data bytes
    [info] roc_sndio: [sox] wav.c:         7208064 Samps/chans
    [info] roc_sndio: [sox] wav.c: Searching for 4c 49 53 54
    [info] roc_sndio: sox source: in_bits=16 out_bits=32 in_rate=44100 out_rate=0 in_ch=2, out_ch=0, is_file=1
    [info] roc_netio: udp sender: opened port 0.0.0.0:48925
    [debug] roc_fec: of encoder: initializing Reed-Solomon encoder
    [debug] roc_sndio: pump: starting
    [debug] roc_netio: transceiver: starting event loop
    [debug] roc_packet: router: detected new stream: source=1995914915 flags=0x8u
    [debug] roc_packet: router: detected new stream: source=0 flags=0x10u
    ...
