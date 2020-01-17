roc-send
********

SYNOPSIS
========

**roc-send** *OPTIONS*

DESCRIPTION
===========

Send real-time audio stream from a file or an audio device to a remote receiver.

Options
-------

-h, --help                Print help and exit
-V, --version             Print version and exit
-v, --verbose             Increase verbosity level (may be used multiple times)
-L, --list-supported      list supported schemes and formats
-i, --input=IO_URI        Input file or device URI
--input-format=FORMAT     Force input file format
-s, --source=PORT         Remote source port triplet
-r, --repair=PORT         Remote repair port triplet
--broadcast               Allow broadcast destination port addresses (default=off)
--nbsrc=INT               Number of source packets in FEC block
--nbrpr=INT               Number of repair packets in FEC block
--packet-length=STRING    Outgoing packet length, TIME units
--packet-limit=INT        Maximum packet size, in bytes
--frame-size=INT          Internal frame size, number of samples
--rate=INT                Override input sample rate, Hz
--no-resampling           Disable resampling  (default=off)
--resampler-backend=ENUM  Resampler backend  (possible values="builtin" default=`builtin')
--resampler-profile=ENUM  Resampler profile  (possible values="low", "medium", "high" default=`medium')
--resampler-interp=INT    Resampler sinc table precision
--resampler-window=INT    Number of samples per resampler window
--interleaving            Enable packet interleaving  (default=off)
--poisoning               Enable uninitialized memory poisoning (default=off)
--color=ENUM              Set colored logging mode for stderr output (possible values="auto", "always", "never" default=`auto')

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

Port
----

*PORT* should be in one of the following forms:

- ``protocol:ipv4addr:portnum``
- ``protocol:[ipv6addr]:portnum``

For example:

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

Time units
----------

*TIME* should have one of the following forms:
  123ns, 123us, 123ms, 123s, 123m, 123h

EXAMPLES
========

Send WAV file:

.. code::

    $ roc-send -vv -i file:./input.wav -s rtp+rs8m:192.168.0.3:10001 -r rs8m:192.168.0.3:10002

Send WAV file to an IPv6 receiver:

.. code::

    $ roc-send -vv -i file:./input.wav -s rtp+rs8m:[2001:db8::]:10001 -r rs8m:[2001:db8::]:10002

Send WAV file to a broadcast address:

.. code::

    $ roc-send -vv -i file:./input.wav -s rtp+rs8m:192.168.0.255:10001 -r rs8m:192.168.0.255:10002 --broadcast

Send WAV from stdin:

.. code::

    $ roc-send -vv -i file:- --input-format wav \
      -s rtp+rs8m:192.168.0.3:10001 -r rs8m:192.168.0.3:10002 < ./input.wav

Send WAV file, specify full URI:

.. code::

    $ roc-send -vv -i file:///home/user/input.wav -s rtp+rs8m:192.168.0.3:10001 -r rs8m:192.168.0.3:10002

Capture sound from the default audio device:

.. code::

    $ roc-send -vv -s rtp+rs8m:192.168.0.3:10001 -r rs8m:192.168.0.3:10002

Capture sound from the default ALSA device:

.. code::

    $ roc-send -vv -i alsa://default -s rtp+rs8m:192.168.0.3:10001 -r rs8m:192.168.0.3:10002

Capture sound from a specific PulseAudio device:

.. code::

    $ roc-send -vv -i pulse://alsa_input.pci-0000_00_1f.3.analog-stereo \
      -s rtp+rs8m:192.168.0.3:10001 -r rs8m:192.168.0.3:10002

Force a specific rate on the input device:

.. code::

    $ roc-send -vv --rate=44100 -s rtp+rs8m:192.168.0.3:10001 -r rs8m:192.168.0.3:10002

Select the LDPC-Staircase FEC scheme and a larger block size:

.. code::

    $ roc-send -vv -i file:./input.wav -s rtp+ldpc:192.168.0.3:10003 -r ldpc:192.168.0.3:10004 \
        --nbsrc=1000 --nbrpr=500

Select bare RTP without FEC:

.. code::

    $ roc-send -vv -i file:./input.wav -s rtp:192.168.0.3:10005

Select resampler profile:

.. code::

    $ roc-send -vv --resampler-profile=high -s rtp+rs8m:192.168.0.3:10001 -r rs8m:192.168.0.3:10002

SEE ALSO
========

:manpage:`roc-recv(1)`, :manpage:`roc-conv(1)`, :manpage:`sox(1)`, the Roc web site at https://roc-project.github.io/

BUGS
====

Please report any bugs found via GitHub (https://github.com/roc-project/roc/).

AUTHORS
=======

See `authors <https://roc-project.github.io/roc/docs/about_project/authors.html>`_ page on the website for a list of maintainers and contributors.
