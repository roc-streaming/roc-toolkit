roc-recv
********

.. only:: html

  .. contents:: Table of contents:
     :local:
     :depth: 1

SYNOPSIS
========

**roc-recv** *OPTIONS*

DESCRIPTION
===========

Receive audio stream(s) from remote sender(s) and write to an audio device or file.

.. begin_options

General options
---------------

-h, --help            Print help and exit
-V, --version         Print version and exit
-v, --verbose         Increase verbosity level (may be used multiple times)
--color=ENUM          Set colored logging mode for stderr output (possible values="auto", "always", "never" default=`auto')
-L, --list-supported  List supported protocols, formats, etc.

Operation options
-----------------

-1, --oneshot  Exit when last connection is closed (default=off)

Output options
--------------

-o, --output=IO_URI        Output file or device URI
--io-encoding=IO_ENCODING  Output device encoding
--io-latency=TIME          Output device latency, TIME units
--io-frame-len=TIME        Output frame length, TIME units

Backup input options
--------------------

--backup=IO_URI  Backup file URI (used as input when there are no connections)

Network options
---------------

-s, --source=NET_URI   Local source endpoint to listen on
-r, --repair=NET_URI   Local repair endpoint to listen on
-c, --control=NET_URI  Local control endpoint to listen on
--miface=IPADDR        IPv4 or IPv6 address of the network interface on which to join the multicast group
--reuseaddr            Enable SO_REUSEADDR when binding sockets

Decoding options
----------------

--packet-encoding=PKT_ENCODING  Custom network packet encoding(s) (may be used multiple times)
--plc=ENUM                      Algorithm to mask unrecoverable packet losses (possible values="none", "beep" default=`none')
--resampler-backend=ENUM        Resampler backend  (possible values="auto", "builtin", "speex", "speexdec" default=`auto')
--resampler-profile=ENUM        Resampler profile  (possible values="low", "medium", "high" default=`medium')

Latency options
---------------

--target-latency=TIME     Target latency, TIME units or 'auto' for adaptive mode  (default=`auto')
--latency-tolerance=TIME  Maximum deviation from target latency, TIME units
--start-latency=TIME      Starting target latency in adaptive mode, TIME units
--min-latency=TIME        Minimum target latency in adaptive mode, TIME units
--max-latency=TIME        Maximum target latency in adaptive mode, TIME units
--latency-backend=ENUM    Which latency to measure and tune  (possible values="niq" default=`niq')
--latency-profile=ENUM    Latency tuning profile  (possible values="auto", "responsive", "gradual", "intact" default=`auto')

Timeout options
---------------

--no-play-timeout=TIME      No-playback timeout, TIME units
--choppy-play-timeout=TIME  Choppy playback timeout, TIME units

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

``--output`` and ``--backup`` options define device or file URIs.

*IO_URI* should have one of the following forms:

- ``<driver>://<device>`` -- audio device
- ``<driver>://default`` -- default audio device for given device type
- ``file:///<abs>/<path>`` -- absolute file path
- ``file://localhost/<abs>/<path>`` -- absolute file path (alternative form for RFC conformance; only "localhost" host is supported)
- ``file:/<abs>/<path>`` -- absolute file path (alternative form)
- ``file:<rel>/<path>`` -- relative file path
- ``file://-`` -- stdout
- ``file:-`` -- stdout (alternative form)

Examples:

- ``pulse://default``
- ``pulse://alsa_output.pci-0000_00_1f.3.analog-stereo``
- ``alsa://hw:1,0``
- ``file:///home/user/test.wav``
- ``file://localhost/home/user/test.wav``
- ``file:/home/user/test.wav``
- ``file:./test.wav``
- ``file:-``

The list of supported schemes and file formats can be retrieved using ``--list-supported`` option.

If the ``--output`` is omitted, default driver and device are selected.
If the ``--backup`` is omitted, no backup source is used.

The path component of the provided URI is `percent-decoded <https://en.wikipedia.org/wiki/Percent-encoding>`_. For convenience, unencoded characters are allowed as well, except that ``%`` should be always encoded as ``%25``.

For example, the file named ``/foo/bar%/[baz]`` may be specified using either of the following URIs: ``file:///foo%2Fbar%25%2F%5Bbaz%5D`` and ``file:///foo/bar%25/[baz]``.

I/O encoding
------------

``--io-encoding`` option allows to explicitly specify encoding of the output file or device.

This option is useful when device supports multiple encodings, or specific file encoding is preferred. Note that I/O encoding may be different from network packet encoding(s). Necessary conversions will be applied automatically.

*IO_ENCODING* should have the following form:

``<format>[@<subformat>]/<rate>/<channels>``

Where:

* ``format`` defines container format, e.g. ``pcm`` (raw samples), ``wav``, ``ogg``
* ``subformat`` is optional format-dependent codec, e.g. ``s16`` for ``pcm`` or ``wav``, and ``vorbis`` for ``ogg``
* ``rate`` defines sample rate in Hertz (number of samples per second), e.g. ``48000``
* ``channels`` defines channel layout, e.g. ``mono`` or ``stereo``

``format``, ``rate``, and ``channels`` may be set to special value ``-``, which means using default value for the specified output device or file format.

Whether ``subformat`` is required, allowed, and what values are accepted, depends on ``format``.

Examples:

* ``pcm@s16/44100/mono`` -- PCM, 16-bit native-endian integers, 44.1KHz, 1 channel
* ``pcm@f32_le/48000/stereo`` -- PCM, 32-bit little-endian floats, 48KHz, 2 channels
* ``pcm@s24_4be/-/-`` -- PCM, 24-bit integers packed into 4-byte big-endian frames, default rate and channels
* ``wav/-/-`` -- WAV, default sample width, rate, and channels
* ``wav@s24/-/-`` -- WAV, 24-bit samples, default rate and channels
* ``flac@s16/48000/stereo`` -- FLAC, 16-bit samples, 48KHz, 2 channels
* ``ogg/48000/stereo`` -- OGG, default codec, 48KHz, 2 channels
* ``ogg@vorbis/48000/stereo`` -- OGG, Vorbis codec, 48KHz, 2 channels

Devices (``pulse://``, ``alsa://``, etc.) usually support only ``pcm`` format. Files (``file://``) support a lot of different formats.

The list of supported formats, sub-formats, and channel layouts can be retrieved using ``--list-supported`` option.

I/O latency and frame
---------------------

``--io-latency`` option defines I/O buffer size for the output device. It can't be used if output is a file.

Exact semantics depends on sound system and sound card driver. For some drivers, the size of this buffer covers both software ring buffer and hardware DAC buffer, for others it covers only software buffer.

``--io-frame-len`` option defines chunk size for a single I/O operation. I/O latency is typically a multiple of I/O frame size.

Higher values increase robustness, and lower values decrease overall end-to-end latency. If not specified, some ""medium" values are selected depending on driver.

Network URI
-----------

``--source``, ``--repair``, and ``--control`` options define network endpoints on which to receive the traffic.

*ENDPOINT_URI* should have the following form:

``<protocol>://<host>[:<port>][/<path>][?<query>]``

Examples:

- ``rtsp://localhost:123/path?query``
- ``rtp+rs8m://localhost:123``
- ``rtp://0.0.0.0:123``
- ``rtp://[::1]:123``
- ``rtcp://0.0.0.0:123``

The list of supported protocols can be retrieved using ``--list-supported`` option.

The host field should be either FQDN (domain name), or IPv4 address, or IPv6 address in square brackets. It may be ``0.0.0.0`` (for IPv4) or ``[::]`` (for IPv6) to bind endpoint to all network interfaces.

The port field can be omitted if the protocol defines standard port. Otherwise, it is mandatory. It may be set to zero to bind endpoint to a randomly chosen ephemeral port.

The path and query fields are allowed only for protocols that support them, e.g. for RTSP.

If FEC is enabled on sender, a pair of a source and repair endpoints should be provided. The two endpoints should use compatible protocols, e.g. ``rtp+rs8m://`` for source endpoint, and ``rs8m://`` for repair endpoint. If FEC is disabled, a single source endpoint should be provided.

Supported source and repair protocols:

- source ``rtp://``, repair none (bare RTP without FEC)
- source ``rtp+rs8m://``, repair ``rs8m://`` (RTP with Reed-Solomon FEC)
- source ``rtp+ldpc://``, repair ``ldpc://`` (RTP with LDPC-Staircase FEC)

In addition, it is recommended to provide control endpoint. It is used to exchange non-media information used to identify session, carry feedback, etc. If no control endpoint is provided, session operates in reduced fallback mode, which may be less robust and may not support all features.

Supported control protocols:

- ``rtcp://``

Packet encodings
----------------

``--packet-encoding`` option allows to specify custom encoding(s) of the network packets.

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

You can use this option several times if different senders use different encodings. Ensure that all encodings has different identifiers, so that receiver can distinguish between them.

Resampler configuration
-----------------------

Receiver uses resampler (a.k.a. sample rate converter) for two purposes:

* to convert between packet encoding sample rate and I/O encoding sample rate, if they're different
* if receiver-side latency tuning is enabled (which is the default), to adjust clock speed dynamically for clock drift compensation

``--resampler-backend`` and ``--resampler-profile`` allow to specify which engine is used for resampling and which quality profile is applied.

A few backends are available:

* ``auto`` -- select most appropriate backend automatically
* ``builtin`` -- CPU-intensive, good-quality, high-precision built-in resampler
* ``speex`` -- fast, good-quality, low-precision resampler based on SpeexDSP
* ``speexdec`` -- very fast, medium-quality, medium-precision resampler combining SpeexDSP for base rate conversion, and decimation for clock drift compensation

Here, quality reflects potential distortions introduced by resampler, and precision reflects how accurately resampler can apply scaling and hence how accurately we can tune latency.

For very low or very precise latency, you usually need to use ``builtin`` backend. If those factors are not critical, you may use ``speex`` resampler to reduce CPU usage. ``speexdec`` backend is a compromise for situations when both CPU usage and latency are critical, and quality is less important.

If receiver-side latency tuning is disabled (by default it's enabled), resampler precision is not relevant, and ``speex`` is almost always the best choice.

Latency configuration
---------------------

This section is relevant when receiver-side latency tuning is enabled (this is default).

By default, latency tuning is performed on receiver side: ``--latency-profile`` is set to ``auto`` on receiver and to ``intact`` on sender. If you want to do it on sender side, you can set ``--latency-profile`` to ``intact`` on receiver and to something else on sender. This is useful when receiver is more CPU-constrained than sender, because latency tuning uses resampler. However, note that sender may perform tuning less accurately, depending on network lag.

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

Timeouts
--------

There are two timeout options determining when to terminate problematic connections:

* ``--no-play-timeout`` -- Terminate connection if there is no playback (i.e. no good packets) during timeout. Allows to detect dead, hanging, or incompatible clients that generate unparseable packets.

* ``--choppy-play-timeout`` -- Terminate connection if there is constant stuttering during this period. Allows to detect situations when playback continues but there are frequent glitches, for example because there is a high ratio of late packets.

Multicast interface
-------------------

If ``--miface`` option is present, it defines an IP address of the network interface on which to join the multicast group. If not present, no multicast group should be joined.

It's not possible to receive multicast traffic without joining a multicast group. The user should either provide multicast interface, or join the group manually using third-party tools.

*IPADDR* should be an IP address of the network interface on which to join the multicast group. It may be ``0.0.0.0`` (for IPv4) or ``::`` (for IPv6) to join the multicast group on all available interfaces.

Although most traffic goes from sender to receiver, there is also feedback traffic from receiver to sender, so both sender and receiver should join multicast group.

Multiple unicast addresses
--------------------------

You can bind receiver to multiple addresses by specifying several sets of endpoints, called "slots".

Each slot has its own ``--source``, ``--repair``, and ``--control`` endpoint and optional ``--miface`` address. All receiver slots should have the same set of endpoint types (source, repair, etc). For example, to bind receiver to 2 addresses, you'll need to specify 2 groups of ``--source``, ``--repair``, and ``--control`` options. Receiver allows slots to use different sets of protocols.

This feature is useful if you want to accept connections from different interfaces or using different protocols.

SO_REUSEADDR
------------

If ``--reuseaddr`` option is provided, ``SO_REUSEADDR`` socket option will be enabled for all sockets (by default it's enabled only for multicast sockets).

For TCP, it allows immediately reusing recently closed socket in TIME_WAIT state, which may be useful you want to be able to restart server quickly.

For UDP, it allows multiple processes to bind to the same address, which may be useful if you're using systemd socket activation.

Regardless of the option, ``SO_REUSEADDR`` is always disabled when binding to ephemeral port.

Backup audio
------------

If ``--backup`` option is given, it defines input file to be played when there are no connected sessions. If it's not given, silence is played instead.

Backup file is restarted from the beginning each time when the last session disconnect. The playback of of the backup file is automatically looped.

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

Bind one bare RTP endpoint on all IPv4 interfaces:

.. code::

    $ roc-recv -vv -s rtp://0.0.0.0:10001

Bind source, repair, and control endpoints to all IPv4 interfaces (but not IPv6):

.. code::

    $ roc-recv -vv -s rtp+rs8m://0.0.0.0:10001 -r rs8m://0.0.0.0:10002 \
        -c rtcp://0.0.0.0:10003

Bind source, repair, and control endpoints to all IPv6 interfaces (but not IPv4):

.. code::

    $ roc-recv -vv -s rtp+rs8m://[::]:10001 -r rs8m://[::]:10002 -c rtcp://[::]:10003

Bind source, repair, and control endpoints to a particular network interface:

.. code::

    $ roc-recv -vv -s rtp+rs8m://192.168.0.3:10001 -r rs8m://192.168.0.3:10002 \
        -c rtcp://192.168.0.3:10003

Bind endpoints to a particular multicast address and join to a multicast group on a particular network interface:

.. code::

    $ roc-recv -vv -s rtp+rs8m://225.1.2.3:10001 -r rs8m://225.1.2.3:10002 \
        -c rtcp://225.1.2.3:10003 \
        --miface 192.168.0.3

Bind two sets ("slots") of source, repair, and control endpoints (six endpoints in total):

.. code::

    $ roc-recv -vv \
        -s rtp+rs8m://192.168.0.3:10001 -r rs8m://192.168.0.3:10002 \
            -c rtcp://192.168.0.3:10003 \
        -s rtp+rs8m://198.214.0.7:10001 -r rs8m://198.214.0.7:10002 \
            -c rtcp://198.214.0.7:10003

I/O examples
------------

Output to the default device (omit ``-o``):

.. code::

    $ roc-recv -vv -s rtp://0.0.0.0:10001

Output to the default ALSA device:

.. code::

    $ roc-recv -vv -s rtp://0.0.0.0:10001 -o alsa://default

Output to a specific PulseAudio device:

.. code::

    $ roc-recv -vv -s rtp://0.0.0.0:10001 -o pulse://alsa_input.pci-0000_00_1f.3.analog-stereo

Output to a file in WAV format (guess format by extension):

.. code::

    $ roc-recv -vv -s rtp://0.0.0.0:10001 -o file:./output.wav

Output to a file in WAV format (specify format manually):

.. code::

    $ roc-recv -vv -s rtp://0.0.0.0:10001 -o file:./output.1 --io-encoding wav/-/-

Output to stdout in WAV format:

.. code::

    $ roc-recv -vv -s rtp://0.0.0.0:10001 -o file:- --io-encoding wav/-/- >./output.wav

Output to a file in WAV format (absolute path):

.. code::

    $ roc-recv -vv -s rtp://0.0.0.0:10001 -o file:///home/user/output.wav

Specify backup file:

.. code::

    $ roc-recv -vv -s rtp://0.0.0.0:10001 --backup file:./backup.wav

Tuning examples
---------------

Force specific encoding on the output device:

.. code::

    $ roc-recv -vv -s rtp://0.0.0.0:10001 \
        --output alsa://hw:1,0 --io-encoding pcm@s32/48000/stereo

Force specific encoding on the output file:

.. code::

    $ roc-recv -vv -s rtp://0.0.0.0:10001 \
        --output file:./output.flac --io-encoding flac@s24/48000/stereo

Use specific encoding for network packets:

.. code::

    $ roc-send -vv -s rtp://192.168.0.3:10001 --packet-encoding 101:pcm@s24/48000/stereo

.. code::

    $ roc-recv -vv -s rtp://0.0.0.0:10001 --packet-encoding 101:pcm@s24/48000/stereo

Select the LDPC-Staircase FEC scheme:

.. code::

    $ roc-send -vv -s rtp+ldpc://192.168.0.3:10001 -r ldpc://192.168.0.3:10002 \
        -c rtcp://192.168.0.3:10003

.. code::

    $ roc-recv -vv -s rtp+ldpc://0.0.0.0:10001 -r ldpc://0.0.0.0:10002 \
        -c rtcp://0.0.0.0:10003

Select fixed streaming latency instead of adaptive latency and low tolerance to latency deviations:

.. code::

    $ roc-recv -vv -s rtp://0.0.0.0:10001 \
        --target-latency=40ms --latency-tolerance 10ms

Select I/O latency and frame length:

.. code::

    $ roc-recv -vv -s rtp://0.0.0.0:10001 \
        --io-latency=20ms --io-frame-len=4ms

Manually specify thresholds for adaptive latency:

.. code::

    $ roc-recv -vv -s rtp://0.0.0.0:10001 \
        --target-latency=auto \
        --start-latency=300ms --min-latency=100ms --max-latency 500ms

Manually specify timeouts:

.. code::

    $ roc-recv -vv -s rtp://0.0.0.0:10001 \
        --no-play-timeout=200ms --choppy-play-timeout=500ms

Manually specify latency tuning parameters:

.. code::

    $ roc-recv -vv -s rtp://0.0.0.0:10001 \
        --latency-backend=niq --latency-profile=gradual

Manually specify resampling parameters:

.. code::

    $ roc-recv -vv -s rtp://0.0.0.0:10001 \
        --resampler-backend=speex --resampler-profile=high

ENVIRONMENT
===========

The following environment variables are supported:

NO_COLOR
    By default, terminal coloring is automatically detected. This environment variable can be set to a non-empty string to disable terminal coloring. It has lower precedence than ``--color`` option.

FORCE_COLOR
    By default, terminal coloring is automatically detected. This environment variable can be set to a positive integer to enable/force terminal coloring. It has lower precedence than  ``NO_COLOR`` variable and ``--color`` option.

SEE ALSO
========

:manpage:`roc-send(1)`, :manpage:`roc-copy(1)`, and the Roc web site at https://roc-streaming.org/

BUGS
====

Please report any bugs found via GitHub (https://github.com/roc-streaming/roc-toolkit/).

AUTHORS
=======

See authors page on the website for a list of maintainers and contributors (https://roc-streaming.org/toolkit/docs/about_project/authors.html).
