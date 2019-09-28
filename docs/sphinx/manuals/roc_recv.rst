roc-recv
********

SYNOPSIS
========

**roc-recv** *OPTIONS*

DESCRIPTION
===========

Receive real-time audio streams from remote senders and write them to a file or an audio device.

Options
-------

-h, --help                Print help and exit
-V, --version             Print version and exit
-v, --verbose             Increase verbosity level (may be used multiple times)
-L, --list-supported      list supported schemes and formats
-o, --output=OUTPUT_URI   Output file or device URI
-f, --format=FORMAT       Force output file format
-s, --source=PORT         Source port triplet (may be used multiple times)
-r, --repair=PORT         Repair port triplet (may be used multiple times)
--miface=IFACE_IP         IP address of the network interface on which to join the multicast group
--sess-latency=STRING     Session target latency, TIME units
--min-latency=STRING      Session minimum latency, TIME units
--max-latency=STRING      Session maximum latency, TIME units
--io-latency=STRING       Playback target latency, TIME units
--np-timeout=STRING       Session no playback timeout, TIME units
--bp-timeout=STRING       Session broken playback timeout, TIME units
--bp-window=STRING        Session breakage detection window, TIME units
--packet-limit=INT        Maximum packet size, in bytes
--frame-size=INT          Internal frame size, number of samples
--rate=INT                Override output sample rate, Hz
--no-resampling           Disable resampling  (default=off)
--resampler-profile=ENUM  Resampler profile  (possible values="low", "medium", "high" default=`medium')
--resampler-interp=INT    Resampler sinc table precision
--resampler-window=INT    Number of samples per resampler window
-1, --oneshot             Exit when last connected client disconnects (default=off)
--poisoning               Enable uninitialized memory poisoning (default=off)
--beeping                 Enable beeping on packet loss  (default=off)

Output URI
----------

``--output`` option requires a device or file URI in one of the following forms:

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
- ``alsa://front:CARD=PCH,DEV=0``
- ``file:///home/user/test.wav``
- ``file://localhost/home/user/test.wav``
- ``file:/home/user/test.wav``
- ``file:./test.wav``
- ``file:-``

The list of supported schemes and file formats can be retrieved using ``--list-supported`` option.

If the ``--output`` is omitted, the default driver and device are selected.

The ``--format`` option can be used to force the output file format. If it is omitted, the file format is auto-detected. This option is always required when the output is stdout.

The path component of the provided URI is `percent-decoded <https://en.wikipedia.org/wiki/Percent-encoding>`_. For convenience, unencoded characters are allowed as well, except that ``%`` should be always encoded as ``%25``.

For example, the file named ``/foo/bar%/[baz]`` may be specified using either of the following URIs: ``file:///foo%2Fbar%25%2F%5Bbaz%5D`` and ``file:///foo/bar%25/[baz]``.

Port
----

*PORT* should be in one of the following forms:

- ``protocol::portnum`` (0.0.0.0 IP address is used)
- ``protocol:ipv4addr:portnum``
- ``protocol:[ipv6addr]:portnum``

For example:

- rtp+rs8m::10001
- rtp+rs8m:127.0.0.1:10001
- rtp+rs8m:[::1]:10001

If FEC is enabled on sender, a pair of a source and repair ports should be used for communication between sender and receiver. If FEC is disabled, a single source port should be used instead.

Supported protocols for source ports:

- rtp (bare RTP, no FEC scheme)
- rtp+rs8m (RTP + Reed-Solomon m=8 FEC scheme)
- rtp+ldpc (RTP + LDPC-Starircase FEC scheme)

Supported protocols for repair ports:

- rs8m (Reed-Solomon m=8 FEC scheme)
- ldpc (LDPC-Starircase FEC scheme)

Multicast interface
-------------------

*IFACE_IP* should be an IP address of the network interface on which to join the multicast group. It may be "0.0.0.0" (for IPv4) or "[::]" (for IPv6) to join the multicast group on all available interfaces.

If *IFACE_IP* is not specified and multicast address is used, the user is responsible for joining the multicast group manually.

Time units
----------

*TIME* should have one of the following forms:
  123ns, 123us, 123ms, 123s, 123m, 123h

EXAMPLES
========

Listen on one bare RTP port on all IPv4 interfaces:

.. code::

    $ roc-recv -vv -s rtp::10001

Listen on two ports on all IPv4 interfaces (but not IPv6):

.. code::

    $ roc-recv -vv -s rtp+rs8m::10001 -r rs8m::10002

Listen on two ports on all IPv6 interfaces (but not IPv4):

.. code::

    $ roc-recv -vv -s rtp+rs8m:[::]:10001 -r rs8m:[::]:10002

Listen on two ports on all IPv4 interfaces (using LDPC scheme)

.. code::

    $ roc-recv -vv -s rtp+ldpc::10001 -r ldpc::10002

Listen on two ports on a particular interface:

.. code::

    $ roc-recv -vv -s rtp+rs8m:192.168.0.3:10001 -r rs8m:192.168.0.3:10002

Listen on two ports on a particular multicast address (but not join to a multicast group):

.. code::

    $ roc-recv -vv -s rtp+rs8m:225.1.2.3:10001 -r rs8m:225.1.2.3:10002

Listen on two ports on a particular multicast address and join to a multicast group on all interfaces:

.. code::

    $ roc-recv -vv -s rtp+rs8m:225.1.2.3:10001 -r rs8m:225.1.2.3:10002 --miface 0.0.0.0

Listen on two ports on a particular multicast address and join to a multicast group on a particular interface:

.. code::

    $ roc-recv -vv -s rtp+rs8m:225.1.2.3:10001 -r rs8m:225.1.2.3:10002 --miface 192.168.0.3

Output to the default ALSA device:

.. code::

    $ roc-recv -vv -o alsa://default -s rtp+rs8m::10001 -r rs8m::10002

Output to a specific PulseAudio device:

.. code::

    $ roc-recv -vv -o pulse://alsa_input.pci-0000_00_1f.3.analog-stereo -s rtp+rs8m::10001 -r rs8m::10002

Output to a file in WAV format (guess format by extension):

.. code::

    $ roc-recv -vv -o file:./output.wav -s rtp+rs8m::10001 -r rs8m::10002

Output to a file in WAV format (specify format manually):

.. code::

    $ roc-recv -vv -o file:./output -f wav -s rtp+rs8m::10001 -r rs8m::10002

Output to stdout in WAV format:

.. code::

    $ roc-recv -vv -o file:- -f wav -s rtp+rs8m::10001 -r rs8m::10002 > ./output.wav

Output to a file in WAV format, specify full URI:

.. code::

    $ roc-recv -vv -o file:///home/user/output.wav -s rtp+rs8m::10001 -r rs8m::10002

Force a specific rate on the output device:

.. code::

    $ roc-recv -vv --rate=44100 -s rtp+rs8m::10001 -r rs8m::10002

Select higher session latency and timeouts:

.. code::

    $ roc-recv -vv -s rtp+rs8m::10001 -r rs8m::10002 \
      --sess-latency=5s --min-latency=-1s --max-latency=10s --np-timeout=10s --bp-timeout=10s

Select higher I/O latency:

.. code::

    $ roc-recv -vv --io-latency=200ms -s rtp+rs8m::10001 -r rs8m::10002

Select resampler profile:

.. code::

    $ roc-recv -vv --resampler-profile=high -s rtp+rs8m::10001 -r rs8m::10002

SEE ALSO
========

:manpage:`roc-send(1)`, :manpage:`roc-conv(1)`, :manpage:`sox(1)`, the Roc web site at https://roc-project.github.io/

BUGS
====

Please report any bugs found via GitHub (https://github.com/roc-project/roc/).

AUTHORS
=======

See `authors <https://roc-project.github.io/roc/docs/about_project/authors.html>`_ page on the website for a list of maintainers and contributors.
