roc-recv
********

SYNOPSIS
========

**roc-recv** *OPTIONS*

DESCRIPTION
===========

Receive audio streams from remote senders and write them to an audio device or file.

Options
-------

-h, --help                   Print help and exit
-V, --version                Print version and exit
-v, --verbose                Increase verbosity level (may be used multiple times)
-L, --list-supported         list supported schemes and formats
-o, --output=IO_URI          Output file or device URI
--output-format=FILE_FORMAT  Force output file format
--backup=IO_URI              Backup file or device URI (if set, used when there are no sessions)
--backup-format=FILE_FORMAT  Force backup file format
-s, --source=ENDPOINT_URI    Local source endpoint
-r, --repair=ENDPOINT_URI    Local repair endpoint
--miface=MIFACE              IPv4 or IPv6 address of the network interface on which to join the multicast group
--sess-latency=STRING        Session target latency, TIME units
--min-latency=STRING         Session minimum latency, TIME units
--max-latency=STRING         Session maximum latency, TIME units
--io-latency=STRING          Playback target latency, TIME units
--np-timeout=STRING          Session no playback timeout, TIME units
--bp-timeout=STRING          Session broken playback timeout, TIME units
--bp-window=STRING           Session breakage detection window, TIME units
--packet-limit=INT           Maximum packet size, in bytes
--frame-limit=INT            Maximum internal frame size, in bytes
--frame-length=TIME          Duration of the internal frames, TIME units
--rate=INT                   Override output sample rate, Hz
--no-resampling              Disable resampling  (default=off)
--resampler-backend=ENUM     Resampler backend  (possible values="builtin" default=`builtin')
--resampler-profile=ENUM     Resampler profile  (possible values="low", "medium", "high" default=`medium')
--resampler-interp=INT       Resampler sinc table precision
--resampler-window=INT       Number of samples per resampler window
-1, --oneshot                Exit when last connected client disconnects (default=off)
--poisoning                  Enable uninitialized memory poisoning (default=off)
--profiling                  Enable self profiling (default=off)
--beeping                    Enable beeping on packet loss  (default=off)
--color=ENUM                 Set colored logging mode for stderr output (possible values="auto", "always", "never" default=`auto')

Endpoint URI
------------

``--source`` and ``--repair`` options define network endpoints on which to receive the traffic.

*ENDPOINT_URI* should have the following form:

``protocol://host[:port][/path][?query]``

Examples:

- ``rtsp://localhost:123/path?query``
- ``rtp+rs8m://localhost:123``
- ``rtp://0.0.0.0:123``
- ``rtp://[::1]:123``

The list of supported protocols can be retrieved using ``--list-supported`` option.

The host field should be either FQDN (domain name), or IPv4 address, or IPv6 address in square brackets. It may be ``0.0.0.0`` (for IPv4) or ``[::]`` (for IPv6) to bind endpoint to all network interfaces.

The port field can be omitted if the protocol defines standard port. Otherwise, it is mandatory. It may be set to zero to bind endpoint to a radomly chosen ephemeral port.

The path and query fields are allowed only for protocols that support them, e.g. for RTSP.

If FEC is enabled on sender, a pair of a source and repair endpoints should be provided. The two endpoints should use compatible protocols, e.g. ``rtp+rs8m://`` for source endpoint, and ``rs8m://`` for repair endpoint. If FEC is disabled, a single source endpoint should be provided.

Supported configurations:

- source ``rtp://``, repair none (bare RTP without FEC)
- source ``rtp+rs8m://``, repair ``rs8m://`` (RTP with Reed-Solomon FEC)
- source ``rtp+ldpc://``, repair ``ldpc://`` (RTP with LDPC-Staircase FEC)

IO URI
------

``--output`` and ``--backup`` options require a device or file URI in one of the following forms:

- ``DEVICE_TYPE://DEVICE_NAME`` -- audio device
- ``DEVICE_TYPE://default`` -- default audio device for given device type
- ``file:///ABS/PATH`` -- absolute file path
- ``file://localhost/ABS/PATH`` -- absolute file path (alternative form; only "localhost" host is supported)
- ``file:/ABS/PATH`` -- absolute file path (alternative form)
- ``file:REL/PATH`` -- relative file path
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

If the ``--output`` is omitted, the default driver and device are selected.
If the ``--backup`` is omitted, no backup source is used.

The ``--output-format`` and ``--backup-format`` options can be used to force the output or backup file format. If the option is omitted, the file format is auto-detected. The option is always required when the output or backup is stdout or stdin.

The path component of the provided URI is `percent-decoded <https://en.wikipedia.org/wiki/Percent-encoding>`_. For convenience, unencoded characters are allowed as well, except that ``%`` should be always encoded as ``%25``.

For example, the file named ``/foo/bar%/[baz]`` may be specified using either of the following URIs: ``file:///foo%2Fbar%25%2F%5Bbaz%5D`` and ``file:///foo/bar%25/[baz]``.

Backup audio
------------

If ``--backup`` option is given, it defines input audio device or file which will be played when there are no connected sessions. If it's not given, silence is played instead.

Backup file is restarted from the beginning each time when the last session disconnect. The playback of of the backup file is automatically looped.

Multicast interface
-------------------

If ``--miface`` option is present, it defines an IP address of the network interface on which to join the multicast group. If not present, no multicast group should be joined.

It's not possible to receive multicast traffic without joining a multicast group. The user should either provide multicast interface, or join the group manually using foreign tools.

*MIFACE* should be an IP address of the network interface on which to join the multicast group. It may be ``0.0.0.0`` (for IPv4) or ``::`` (for IPv6) to join the multicast group on all available interfaces.

Time units
----------

*TIME* should have one of the following forms:
  123ns, 123us, 123ms, 123s, 123m, 123h

EXAMPLES
========

Endpoint examples
-----------------

Bind one bare RTP endpoint on all IPv4 interfaces:

.. code::

    $ roc-recv -vv -s rtp://0.0.0.0:10001

Bind two endpoints to all IPv4 interfaces (but not IPv6):

.. code::

    $ roc-recv -vv -s rtp+rs8m://0.0.0.0:10001 -r rs8m://0.0.0.0:10002

Bind two endpoints to all IPv6 interfaces (but not IPv4):

.. code::

    $ roc-recv -vv -s rtp+rs8m://[::]:10001 -r rs8m://[::]:10002

Bind two endpoints to a particular network interface:

.. code::

    $ roc-recv -vv -s rtp+rs8m://192.168.0.3:10001 -r rs8m://192.168.0.3:10002

Bind two endpoints to a particular multicast address and join to a multicast group on a particular network interface:

.. code::

    $ roc-recv -vv -s rtp+rs8m://225.1.2.3:10001 -r rs8m://225.1.2.3:10002 --miface 192.168.0.3

I/O examples
------------

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

    $ roc-recv -vv -s rtp://0.0.0.0:10001 -o file:./output --output-format wav

Output to stdout in WAV format:

.. code::

    $ roc-recv -vv -s rtp://0.0.0.0:10001 -o file:- --output-format wav >./output.wav

Output to a file in WAV format (absolute path):

.. code::

    $ roc-recv -vv -s rtp://0.0.0.0:10001 -o file:///home/user/output.wav

Specify backup file:

.. code::

    $ roc-recv -vv -s rtp://0.0.0.0:10001 --backup file:./backup.wav

Tuning examples
---------------

Force a specific rate on the output device:

.. code::

    $ roc-recv -vv -s rtp://0.0.0.0:10001 --rate=44100

Select the LDPC-Staircase FEC scheme:

.. code::

    $ roc-recv -vv -s rtp+ldpc://0.0.0.0:10001 -r ldpc://0.0.0.0:10002

Select higher session latency and timeouts:

.. code::

    $ roc-recv -vv -s rtp://0.0.0.0:10001 \
        --sess-latency=5s --min-latency=-1s --max-latency=10s --np-timeout=10s --bp-timeout=10s

Select higher I/O latency:

.. code::

    $ roc-recv -vv -s rtp://0.0.0.0:10001 \
        --io-latency=200ms

Select resampler profile:

.. code::

    $ roc-recv -vv -s rtp://0.0.0.0:10001 \
        --resampler-profile=high

SEE ALSO
========

:manpage:`roc-send(1)`, and the Roc web site at https://roc-project.github.io/

BUGS
====

Please report any bugs found via GitHub (https://github.com/roc-project/roc/).

AUTHORS
=======

See `authors <https://roc-project.github.io/roc/docs/about_project/authors.html>`_ page on the website for a list of maintainers and contributors.
