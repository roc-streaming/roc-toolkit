roc-send
********

.. only:: html

  .. contents:: Table of contents:
     :local:
     :depth: 1

SYNOPSIS
========

**roc-send** *OPTIONS*

DESCRIPTION
===========

Read audio stream from an audio device or file and send to remote receiver(s).

.. begin_options

General options
---------------

-h, --help            Print help and exit
-V, --version         Print version and exit
-v, --verbose         Increase verbosity level (may be used multiple times)
--color=ENUM          Set colored logging mode for stderr output (possible values="auto", "always", "never" default=`auto')
-L, --list-supported  List supported protocols, formats, etc.

Input options
-------------

-i, --input=IO_URI         Input file or device URI
--io-encoding=IO_ENCODING  Input device encoding
--io-latency=TIME          Input device latency, TIME units
--io-frame-len=TIME        Input frame length, TIME units

Network options
---------------

-s, --source=NET_URI   Remote source endpoint to connect to
-r, --repair=NET_URI   Remote repair endpoint to connect to
-c, --control=NET_URI  Remote control endpoint to connect to
--miface=IPADDR        IPv4 or IPv6 address of the network interface on which to join the multicast group
--reuseaddr            Enable SO_REUSEADDR when binding sockets

Encoding options
----------------

--packet-encoding=PKT_ENCODING  Custom network packet encoding
--packet-len=TIME               Network packet length, TIME units
--fec-encoding=FEC_ENCODING     FEC encoding, 'auto' to auto-detect from network endpoints
--fec-block-src=INT             Number of source packets in FEC block
--fec-block-rpr=INT             Number of repair packets in FEC block
--resampler-backend=ENUM        Resampler backend  (possible values="auto", "builtin", "speex", "speexdec" default=`auto')
--resampler-profile=ENUM        Resampler profile  (possible values="low", "medium", "high" default=`medium')

Latency options (for sender-side latency tuning)
------------------------------------------------

--target-latency=TIME     Target latency, TIME units or 'auto' for adaptive mode
--latency-tolerance=TIME  Maximum deviation from target latency, TIME units
--start-latency=TIME      Starting target latency in adaptive mode, TIME units
--min-latency=TIME        Minimum target latency in adaptive mode, TIME units
--max-latency=TIME        Maximum target latency in adaptive mode, TIME units
--latency-backend=ENUM    Which latency to measure and tune  (possible values="niq" default=`niq')
--latency-profile=ENUM    Latency tuning profile  (possible values="responsive", "gradual", "intact" default=`intact')

Memory options
--------------

--max-packet-size=SIZE  Maximum network packet size, SIZE units
--max-frame-size=SIZE   Maximum I/O and processing frame size, SIZE units

Debugging options
-----------------

--prof       Enable self-profiling  (default=off)
--dump=PATH  Dump run-time metrics to specified CSV file

.. end_options

DETAILS
=======

I/O URI
-------

``--input`` option defines input device or file URI.

*IO_URI* should have one of the following forms:

- ``<driver>://<device>`` -- specific driver and device
- ``<driver>://default`` -- default device for given driver
- ``file:///<abs>/<path>`` -- absolute file path
- ``file://localhost/<abs>/<path>`` -- absolute file path (alternative form for RFC conformance; only "localhost" host is supported)
- ``file:/<abs>/<path>`` -- absolute file path (alternative form)
- ``file:<rel>/<path>`` -- relative file path
- ``file://-`` -- stdin
- ``file:-`` -- stdin (alternative form)

Examples:

- ``pulse://default``
- ``pulse://alsa_input.pci-0000_00_1f.3.analog-stereo``
- ``alsa://hw:1,0``
- ``file:///home/user/test.wav``
- ``file://localhost/home/user/test.wav``
- ``file:/home/user/test.wav``
- ``file:./test.wav``
- ``file:-``

The list of supported schemes and file formats can be retrieved using ``--list-supported`` option.

If the ``--input`` is omitted, the default driver and device are selected.

The path component of the provided URI is `percent-decoded <https://en.wikipedia.org/wiki/Percent-encoding>`_. For convenience, unencoded characters are allowed as well, except that ``%`` should be always encoded as ``%25``.

For example, the file named ``/foo/bar%/[baz]`` may be specified using either of the following URIs: ``file:///foo%2Fbar%25%2F%5Bbaz%5D`` and ``file:///foo/bar%25/[baz]``.

I/O encoding
------------

``--io-encoding`` option allows to explicitly specify encoding of the input file or device.

This option is useful when device supports multiple encodings, or file encoding can't be detected automatically (e.g. file doesn't have extension or uses header-less format like raw PCM). Note that I/O encoding may be different from network packet encoding. Necessary conversions will be applied automatically.

*IO_ENCODING* should have the following form:

``<format>[@<subformat>]/<rate>/<channels>``

Where:

* ``format`` defines container format, e.g. ``pcm`` (raw samples), ``wav``, ``ogg``
* ``subformat`` is optional format-dependent codec, e.g. ``s16`` for ``pcm`` or ``wav``, and ``vorbis`` for ``ogg``
* ``rate`` defines sample rate in Hertz (number of samples per second), e.g. ``48000``
* ``channels`` defines channel layout, e.g. ``mono`` or ``stereo``

``format``, ``rate``, and ``channels`` may be set to special value ``-``, which means using default value for input device, or auto-detect value for input file.

Whether ``subformat`` is required, allowed, and what values are accepted, depends on ``format``.

Examples:

* ``pcm@s16/44100/mono`` -- PCM, 16-bit native-endian integers, 44.1KHz, 1 channel
* ``pcm@f32_le/48000/stereo`` -- PCM, 32-bit little-endian floats, 48KHz, 2 channels
* ``wav/-/-`` -- WAV file, auto-detect sub-format, rate, channels
* ``flac-/-/-`` -- FLAC file, auto-detect sub-format, rate, channels

Devices (``pulse://``, ``alsa://``, etc.) usually support only ``pcm`` format. Files (``file://``) support a lot of different formats.

The list of supported formats, sub-formats, and channel layouts can be retrieved using ``--list-supported`` option.

I/O latency and frame
---------------------

``--io-latency`` option defines I/O buffer size for the input device. It can't be used if input is a file.

Exact semantics depends on sound system and sound card driver. For some drivers, the size of this buffer covers both software ring buffer and hardware ADC buffer, for others it covers only software buffer.

``--io-frame-len`` option defines chunk size for a single I/O operation. I/O latency is typically a multiple of I/O frame size.

Higher values increase robustness, and lower values decrease overall end-to-end latency. If not specified, some ""medium" values are selected depending on driver.

Network URI
-----------

``--source``, ``--repair``, and ``--control`` options define network endpoints to which to send the traffic.

*NET_URI* should have the following form:

``<protocol>://<host>[:<port>][/<path>][?<query>]``

Examples:

- ``rtsp://localhost:123/some_path?some_query``
- ``rtp+rs8m://localhost:123``
- ``rtp://127.0.0.1:123``
- ``rtp://[::1]:123``
- ``rtcp://10.9.8.3:123``

The list of supported protocols can be retrieved using ``--list-supported`` option.

The host field should be either FQDN (domain name), or IPv4 address, or IPv6 address in square brackets.

The port field can be omitted if the protocol defines standard port. Otherwise, it is mandatory.

The path and query fields are allowed only for protocols that support them, e.g. for RTSP.

If FEC is enabled, a pair of a source and repair endpoints should be provided. The two endpoints should use compatible protocols, e.g. ``rtp+rs8m://`` for source endpoint, and ``rs8m://`` for repair endpoint. If FEC is disabled, a single source endpoint should be provided.

Supported source and repair protocols:

- source ``rtp://``, repair none (bare RTP without FEC)
- source ``rtp+rs8m://``, repair ``rs8m://`` (RTP with Reed-Solomon FEC)
- source ``rtp+ldpc://``, repair ``ldpc://`` (RTP with LDPC-Staircase FEC)

In addition, it is highly recommended to provide control endpoint. It is used to exchange non-media information used to identify session, carry feedback, etc. If no control endpoint is provided, session operates in reduced fallback mode, which may be less robust and may not support all features.

Supported control protocols:

- ``rtcp://``

Packet encoding
---------------

``--packet-encoding`` option allows to specify custom encoding of the network packets.

*PKT_ENCODING* is similar to *IO_ENCODING*, but adds numeric encoding identifier:

``<id>:<format>[@<subformat>]/<rate>/<channels>``

Where:

* ``id`` is an arbitrary number in range 100..127, which should uniquely identify encoding on all related senders and receivers
* ``format`` defines container format, e.g. ``pcm`` (raw samples), ``flac``
* ``subformat`` is optional format-dependent codec, e.g. ``s16`` for ``pcm`` or ``flac``
* ``rate`` defines sample rate in Hertz (number of samples per second), e.g. ``48000``
* ``channels`` defines channel layout, e.g. ``mono`` or ``stereo``

Whether ``subformat`` is required, allowed, and what values are accepted, depends on ``format``.

Examples:

* ``101:pcm@s24/44100/mono`` -- PCM, 24-bit network-endian integers, 44.1KHz, 1 channel
* ``102:pcm@f32/48000/stereo`` -- PCM, 32-bit network-endian floats, 48KHz, 2 channels
* ``103:flac@s16/48000/stereo`` -- FLAC, 16-bit precision, 48KHz, 2 channels

The list of supported formats and channel layouts can be retrieved using ``--list-supported`` option.

If you specify custom packet encoding on sender(s), and don't use signaling protocol like RTSP, you need to specify **exactly same encoding(s)** on receiver, with matching identifiers and parameters.

FEC encoding
------------

``--fec-encoding`` option allows to explicitly specify FEC codec for redundancy packets (used for loss recovery on receiver).

*FEC_ENCODING* supports the following values:

* ``auto`` -- automatically detect FEC encoding from protocols used for ``--source`` and ``--repair`` encodings
* ``none`` -- don't use FEC
* ``rs8m`` -- Reed-Solomon codec -- good for small block size / latency, requires more CPU
* ``ldpc`` -- LDPC-Staircase codec -- good for large block size / latency, requires less CPU

The list of supported FEC encodings and related protocols ``--list-supported`` option.

Note that every FEC encoding requires specific ``--source`` and ``--repair`` protocols to be used on both sender and receiver.

Packet and FEC block size
-------------------------

``--packet-len`` option defines length of a single network packet. Smaller packet lengths allow lower and more precise latency, but increase network overhead and increase risk of packet losses and delays on poor networks.

``--fec-block-src`` and ``--fec-block-rpr`` options define number of source and repair packets, respectively, in FEC block size.

If FEC is enabled (automatically or explicitly via ``--fec-encoding``), packets are grouped into blocks of size defined by ``--fec-block-src``. For each such block, additional redundancy packets are generated, of quantity defined by ``--fec-block-rpr``.

Higher value for ``--fec-block-src`` allows to recover packets even on long burst losses or delays, however requires latency to be higher than FEC block size. Higher value for ``--fec-block-rpr`` allows to recover packets on higher loss ratios, but increases bandwidth and may increase loss or delay ratio on weak networks.

Resampler configuration
-----------------------

Sender uses resampler (a.k.a. sample rate converter) for two purposes:

* to convert between packet encoding sample rate and I/O encoding sample rate, if they're different
* if sender-side latency tuning is enabled (disabled by default), to adjust clock speed dynamically for clock drift compensation

``--resampler-backend`` and ``--resampler-profile`` allow to specify which engine is used for resampling and which quality profile is applied.

A few backends are available:

* ``auto`` -- select most appropriate backend automatically
* ``builtin`` -- CPU-intensive, good-quality, high-precision built-in resampler
* ``speex`` -- fast, good-quality, low-precision resampler based on SpeexDSP
* ``speexdec`` -- very fast, medium-quality, medium-precision resampler combining SpeexDSP for base rate conversion with decimation for clock drift compensation

Here, quality reflects potential distortions introduced by resampler, and precision reflects how accurately resampler can apply scaling and hence how accurately we can tune latency.

For very low or very precise latency, you usually need to use ``builtin`` backend. If those factors are not critical, you may use ``speex`` resampler to reduce CPU usage. ``speexdec`` backend is a compromise for situations when both CPU usage and latency are critical, and quality is less important.

If sender-side latency tuning is disabled (which is the default), resampler precision is not relevant, and ``speex`` is almost always the best choice.

Latency configuration
---------------------

This section is relevant when sender-side latency tuning is enabled (**disabled by default**).

By default, latency tuning is performed on receiver side: ``--latency-profile`` is set to ``auto`` on receiver and to ``intact`` on sender. If you want to do it on sender side, you can set ``--latency-profile`` to ``intact`` on receiver and to something else on sender. This is useful when receiver is more CPU-constrained than sender, because latency tuning uses resampler.

Sender-side latency tuning requires latency parameters (target, start, min, and max latency) to **match on receiver and sender**. Also note that sender may perform tuning less accurately, depending on network lag.

``--target-latency`` option defines the latency value to maintain, as measured by the ``--latency-backend``:

* If value is provided, *fixed latency* mode is activated. The latency starts from ``--target-latency`` and is kept close to that value.

* If option is omitted or set to ``auto``, *adaptive latency* mode is activated. The latency is chosen dynamically. Initial latency is ``--start-latency``, and the allowed range is ``--min-latency`` to ``--max-latency``.

``--latency-tolerance`` option defines maximum allowed deviation of the actual latency from the (current) target latency. If this limit is exceeded for some reason (typically due to poor network conditions), connection is restarted.

How latency is measured (and so which latency is tuned) is defined by ``--latency-backend`` option. The following backends are available:

* ``niq`` --  In this mode, latency is defined as the length of network incoming queue on receiver. Playback speed lock is adjusted to keep queue length close to configured target latency. This backend synchronizes only clock speed, but not position; different receivers will have different (constant, on average) delays.

How latency is tuned is defines by ``--latency-profile`` option:

* ``auto`` -- Automatically select profile based on target latency.
* ``responsive`` -- Adjust clock speed quickly and accurately. Requires good network conditions. Allows very low latencies.
* ``gradual`` -- Adjust clock speed slowly and smoothly. Tolerates very high network jitter, but doesn't allow low latencies.
* ``intact`` -- Do not adjust clock speed at all.

Multicast interface
-------------------

If ``--miface`` option is present, it defines an IP address of the network interface on which to join the multicast group. If not present, no multicast group should be joined.

It's not possible to receive multicast traffic without joining a multicast group. The user should either provide multicast interface, or join the group manually using third-party tools.

*IPADDR* should be an IP address of the network interface on which to join the multicast group. It may be ``0.0.0.0`` (for IPv4) or ``::`` (for IPv6) to join the multicast group on all available interfaces.

Although most traffic goes from sender to receiver, there is also feedback traffic from receiver to sender, so both sender and receiver should join multicast group.

Multiple unicast addresses
--------------------------

You can connect sender to multiple receivers by specifying several sets of endpoints, called "slots".

Each slot has its own ``--source``, ``--repair``, and ``--control`` endpoint and optional ``--miface`` address. All sender slots should have the same set of endpoint types (source, repair, etc). For example, to connect sender to 2 receivers, you'll need to specify 2 groups of ``--source``, ``--repair``, and ``--control`` options. Sender requires all slots to use the same set of protocols.

This feature is useful when you have static and small set of receivers and can't or don't want to configure multicast.

SO_REUSEADDR
------------

If ``--reuseaddr`` option is provided, ``SO_REUSEADDR`` socket option will be enabled for all sockets.

For TCP, it allows immediately reusing recently closed socket in TIME_WAIT state, which may be useful you want to be able to restart server quickly. For UDP, it allows multiple processes to bind to the same address, which may be useful if you're using systemd socket activation.

Regardless of the option, ``SO_REUSEADDR`` is always disabled when binding to ephemeral port.

Time and size units
-------------------

*TIME* defines duration with nanosecond precision.

It should have one of the following forms:
  123ns; 1.23us; 1.23ms; 1.23s; 1.23m; 1.23h;

*SIZE* defines byte size and should have one of the following forms:
  123; 1.23K; 1.23M; 1.23G;

EXAMPLES
========

Endpoint examples
-----------------

Send file to receiver with one bare RTP endpoint:

.. code::

    $ roc-send -vv -i file:./input.wav -s rtp://192.168.0.3:10001

Send file to receiver with IPv4 source, repair, and control endpoints:

.. code::

    $ roc-send -vv -i file:./input.wav -s rtp+rs8m://192.168.0.3:10001 \
        -r rs8m://192.168.0.3:10002 -c rtcp://192.168.0.3:10003

Send file to receiver with IPv6 source, repair, and control endpoints:

.. code::

    $ roc-send -vv -i file:./input.wav -s rtp+rs8m://[2001:db8::]:10001 \
        -r rs8m://[2001:db8::]:10002 -r rtcp://[2001:db8::]:10003

Send file to two destinations ("slots"), each with three endpoints:

.. code::

    $ roc-send -vv \
        -i file:./input.wav \
        -s rtp+rs8m://192.168.0.3:10001 -r rs8m://192.168.0.3:10002 \
            -c rtcp://192.168.0.3:10003 \
        -s rtp+rs8m://198.214.0.7:10001 -r rs8m://198.214.0.7:10002 \
            -c rtcp://198.214.0.7:10003

I/O examples
------------

Capture sound from the default device (omit ``-i``):

.. code::

    $ roc-send -vv -s rtp://192.168.0.3:10001

Capture sound from the default ALSA device:

.. code::

    $ roc-send -vv -s rtp://192.168.0.3:10001 -i alsa://default

Capture sound from a specific PulseAudio device:

.. code::

    $ roc-send -vv -s rtp://192.168.0.3:10001 -i pulse://alsa_input.pci-0000_00_1f.3.analog-stereo

Send WAV file (guess format by extension):

.. code::

    $ roc-send -vv -s rtp://192.168.0.3:10001 -i file:./input.wav

Send WAV file (specify format manually):

.. code::

    $ roc-send -vv -s rtp://192.168.0.3:10001 -i file:./input.file --io-encoding wav/-/-

Send WAV from stdin:

.. code::

    $ roc-send -vv -s rtp://192.168.0.3:10001 -i file:- --io-encoding wav/-/- <./input.wav

Send WAV file (specify absolute path):

.. code::

    $ roc-send -vv -s rtp://192.168.0.3:10001 -i file:///home/user/input.wav

Tuning examples
---------------

Force specific encoding on the input device:

.. code::

    $ roc-send -vv -s rtp://192.168.0.3:10001 \
        --input alsa://hw:1,0 --io-encoding pcm@s32/48000/stereo

Force specific encoding on the input file:

.. code::

    $ roc-send -vv -s rtp://192.168.0.3:10001 \
        --input file:./input.pcm --io-encoding pcm@s32/48000/stereo

Use specific encoding for network packets:

.. code::

    $ roc-send -vv -s rtp://192.168.0.3:10001 --packet-encoding 101:pcm@s24/48000/stereo

.. code::

    $ roc-recv -vv -s rtp://0.0.0.0:10001 --packet-encoding 101:pcm@s24/48000/stereo

Select the LDPC-Staircase FEC scheme and a larger block size:

.. code::

    $ roc-send -vv -s rtp+ldpc://192.168.0.3:10001 -r ldpc://192.168.0.3:10002 \
        -c ldpc://192.168.0.3:10003

.. code::

    $ roc-recv -vv -s rtp+ldpc://0.0.0.0:10001 -r ldpc://0.0.0.0:10002 \
        -c rtcp://0.0.0.0:10003

Select smaller packet length and FEC block size:

.. code::

    $ roc-send -vv -i -s rtp+rs8m://192.168.0.3:10001 -r rtp+rs8m://192.168.0.3:10002 \
        --packet-len=2.5ms --fec-block-src=10 --fec-block-rpr=6

Select I/O latency and frame length:

.. code::

    $ roc-send -vv -s rtp://192.168.0.3:10001 \
        --io-latency=20ms --io-frame-len=4ms

Manually specify resampling parameters:

.. code::

    $ roc-send -vv -s rtp://192.168.0.3:10001 \
        --resampler-backend=speex --resampler-profile=high

Perform latency tuning on sender instead of receiver:

.. code::

    $ roc-recv -vv -s rtp+rs8m://0.0.0.0:10001 -r rs8m://0.0.0.0:10002 \
        -c rtcp://0.0.0.0:10003 \
        --latency-profile=intact --target-latency=auto --start-latency=300ms

    $ roc-send -vv -s rtp+rs8m://192.168.0.3:10001 -r rs8m://192.168.0.3:10002 \
        -c rtcp://192.168.0.3:10003 \
        --latency-profile=gradual --target-latency=auto --start-latency=300ms

ENVIRONMENT
===========

The following environment variables are supported:

NO_COLOR
    By default, terminal coloring is automatically detected. This environment variable can be set to a non-empty string to disable terminal coloring. It has lower precedence than ``--color`` option.

FORCE_COLOR
    By default, terminal coloring is automatically detected. This environment variable can be set to a positive integer to enable/force terminal coloring. It has lower precedence than  ``NO_COLOR`` variable and ``--color`` option.

SEE ALSO
========

:manpage:`roc-recv(1)`, :manpage:`roc-copy(1)`, and the Roc web site at https://roc-streaming.org/

BUGS
====

Please report any bugs found via GitHub (https://github.com/roc-streaming/roc-toolkit/).

AUTHORS
=======

See authors page on the website for a list of maintainers and contributors (https://roc-streaming.org/toolkit/docs/about_project/authors.html).
