roc-send
********

SYNOPSIS
========

**roc-send** *OPTIONS*

DESCRIPTION
===========

Read audio stream from an audio device or file and send it to remote receiver.

Options
-------

-h, --help                  Print help and exit
-V, --version               Print version and exit
-v, --verbose               Increase verbosity level (may be used multiple times)
-L, --list-supported        list supported schemes and formats
-i, --input=IO_URI          Input file or device URI
--input-format=FILE_FORMAT  Force input file format
-s, --source=ENDPOINT_URI   Remote source endpoint
-r, --repair=ENDPOINT_URI   Remote repair endpoint
-c, --control=ENDPOINT_URI  Remote control endpoint
--reuseaddr                 enable SO_REUSEADDR when binding sockets
--target-latency=STRING     Target latency, TIME units
--io-latency=STRING         Recording target latency, TIME units
--min-latency=STRING        Minimum allowed latency, TIME units
--max-latency=STRING        Maximum allowed latency, TIME units
--nbsrc=INT                 Number of source packets in FEC block
--nbrpr=INT                 Number of repair packets in FEC block
--packet-len=STRING         Outgoing packet length, TIME units
--frame-len=TIME            Duration of the internal frames, TIME units
--max-packet-size=SIZE      Maximum packet size, in SIZE units
--max-frame-size=SIZE       Maximum internal frame size, in SIZE units
--rate=INT                  Override input sample rate, Hz
--latency-backend=ENUM      Which latency to use in latency tuner (possible values="niq" default=`niq')
--latency-profile=ENUM      Latency tuning profile  (possible values="responsive", "gradual", "intact" default=`intact')
--resampler-backend=ENUM    Resampler backend  (possible values="default", "builtin", "speex", "speexdec" default=`default')
--resampler-profile=ENUM    Resampler profile  (possible values="low", "medium", "high" default=`medium')
--interleaving              Enable packet interleaving  (default=off)
--profiling                 Enable self profiling  (default=off)
--color=ENUM                Set colored logging mode for stderr output (possible values="auto", "always", "never" default=`auto')

Endpoint URI
------------

``--source``, ``--repair``, and ``--control`` options define network endpoints to which to send the traffic.

*ENDPOINT_URI* should have the following form:

``protocol://host[:port][/path][?query]``

Examples:

- ``rtsp://localhost:123/path?query``
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

In addition, it is recommended to provide control endpoint. It is used to exchange non-media information used to identify session, carry feedback, etc. If no control endpoint is provided, session operates in reduced fallback mode, which may be less robust and may not support all features.

Supported control protocols:

- ``rtcp://``

IO URI
------

``--input`` option requires a device or file URI in one of the following forms:

- ``DEVICE_TYPE://DEVICE_NAME`` -- audio device
- ``DEVICE_TYPE://default`` -- default audio device for given device type
- ``file:///ABS/PATH`` -- absolute file path
- ``file://localhost/ABS/PATH`` -- absolute file path (alternative form; only "localhost" host is supported)
- ``file:/ABS/PATH`` -- absolute file path (alternative form)
- ``file:REL/PATH`` -- relative file path
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

The ``--input-format`` option can be used to force the input file format. If it is omitted, the file format is auto-detected. This option is always required when the input is stdin.

The path component of the provided URI is `percent-decoded <https://en.wikipedia.org/wiki/Percent-encoding>`_. For convenience, unencoded characters are allowed as well, except that ``%`` should be always encoded as ``%25``.

For example, the file named ``/foo/bar%/[baz]`` may be specified using either of the following URIs: ``file:///foo%2Fbar%25%2F%5Bbaz%5D`` and ``file:///foo/bar%25/[baz]``.

Multiple slots
--------------

Multiple sets of endpoints can be specified to send media to multiple addresses.

Such endpoint sets are called slots. All slots should have the same set of endpoint types (source, repair, etc) and should use the same protocols for them.

SO_REUSEADDR
------------

If ``--reuseaddr`` option is provided, ``SO_REUSEADDR`` socket option will be enabled for all sockets.

For TCP, it allows immediately reusing recently closed socket in TIME_WAIT state, which may be useful you want to be able to restart server quickly.

For UDP, it allows multiple processes to bind to the same address, which may be useful if you're using systemd socket activation.

Regardless of the option, ``SO_REUSEADDR`` is always disabled when binding to ephemeral port.

Time units
----------

*TIME* should have one of the following forms:
  123ns; 1.23us; 1.23ms; 1.23s; 1.23m; 1.23h;

Size units
----------

*SIZE* should have one of the following forms:
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

Send file to two receivers, each with three endpoints:

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

    $ roc-send -vv -s rtp://192.168.0.3:10001 -i file:./input.file --input-format wav

Send WAV from stdin:

.. code::

    $ roc-send -vv -s rtp://192.168.0.3:10001 -i file:- --input-format wav <./input.wav

Send WAV file (specify absolute path):

.. code::

    $ roc-send -vv -s rtp://192.168.0.3:10001 -i file:///home/user/input.wav

Tuning examples
---------------

Force a specific rate on the input device:

.. code::

    $ roc-send -vv -s rtp://192.168.0.3:10001 --rate=44100

Select the LDPC-Staircase FEC scheme and a larger block size:

.. code::

    $ roc-send -vv -i file:./input.wav -s rtp+ldpc://192.168.0.3:10001 \
        -r ldpc://192.168.0.3:10002 -c ldpc://192.168.0.3:10003 \
        --nbsrc=1000 --nbrpr=500

Select smaller packet length:

.. code::

    $ roc-send -vv -i file:./input.wav -s rtp+ldpc://192.168.0.3:10001 \
        --packet-len 2500us

Select lower I/O latency and frame length:

.. code::

    $ roc-send -vv -s rtp://192.168.0.3:10001 \
        --io-latency=20ms --frame-len 4ms

Manually specify resampling parameters:

.. code::

    $ roc-send -vv -s rtp://192.168.0.3:10001 \
        --resampler-backend=speex --resampler-profile=high

Perform latency tuning on sender instead of receiver:

.. code::

    $ roc-recv -vv -o pulse://default -s rtp+rs8m://0.0.0.0:10001 \
        -r rs8m://0.0.0.0:10002 -c rtcp://0.0.0.0:10003 \
        --latency-profile=intact --target-latency=200ms

    $ roc-send -vv -i file:./input.wav -s rtp+rs8m://192.168.0.3:10001 \
        -r rs8m://192.168.0.3:10002 -c rtcp://192.168.0.3:10003 \
        --latency-profile=gradual --target-latency=200ms

ENVIRONMENT VARIABLES
=====================

The following environment variables are supported:

NO_COLOR
    By default, terminal coloring is automatically detected. This environment variable can be set to a non-empty string to disable terminal coloring. It has lower precedence than ``--color`` option.

FORCE_COLOR
    By default, terminal coloring is automatically detected. This environment variable can be set to a positive integer to enable/force terminal coloring. It has lower precedence than  ``NO_COLOR`` variable and ``--color`` option.

SEE ALSO
========

:manpage:`roc-recv(1)`, and the Roc web site at https://roc-streaming.org/

BUGS
====

Please report any bugs found via GitHub (https://github.com/roc-streaming/roc-toolkit/).

AUTHORS
=======

See authors page on the website for a list of maintainers and contributors (https://roc-streaming.org/toolkit/docs/about_project/authors.html).
