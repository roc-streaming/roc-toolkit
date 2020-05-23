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
--broadcast                 Allow broadcast destination port addresses (default=off)
--nbsrc=INT                 Number of source packets in FEC block
--nbrpr=INT                 Number of repair packets in FEC block
--packet-length=STRING      Outgoing packet length, TIME units
--packet-limit=INT          Maximum packet size, in bytes
--frame-limit=INT           Maximum internal frame size, in bytes
--frame-length=TIME         Duration of the internal frames, TIME units
--rate=INT                  Override input sample rate, Hz
--no-resampling             Disable resampling  (default=off)
--resampler-backend=ENUM    Resampler backend  (possible values="builtin" default=`builtin')
--resampler-profile=ENUM    Resampler profile  (possible values="low", "medium", "high" default=`medium')
--resampler-interp=INT      Resampler sinc table precision
--resampler-window=INT      Number of samples per resampler window
--interleaving              Enable packet interleaving  (default=off)
--poisoning                 Enable uninitialized memory poisoning (default=off)
--profiling                 Enable self profiling (default=off)
--color=ENUM                Set colored logging mode for stderr output (possible values="auto", "always", "never" default=`auto')

Endpoint URI
------------

``--source`` and ``--repair`` options define network endpoints to which to send the traffic.

*ENDPOINT_URI* should have the following form:

``protocol://host[:port][/path][?query]``

Examples:

- ``rtsp://localhost:123/path?query``
- ``rtp+rs8m://localhost:123``
- ``rtp://127.0.0.1:123``
- ``rtp://[::1]:123``

The list of supported protocols can be retrieved using ``--list-supported`` option.

The host field should be either FQDN (domain name), or IPv4 address, or IPv6 address in square brackets.

The port field can be omitted if the protocol defines standard port. Otherwise, it is mandatory.

The path and query fields are allowed only for protocols that support them, e.g. for RTSP.

If FEC is enabled, a pair of a source and repair endpoints should be provided. The two endpoints should use compatible protocols, e.g. ``rtp+rs8m://`` for source endpoint, and ``rs8m://`` for repair endpoint. If FEC is disabled, a single source endpoint should be provided.

Supported configurations:

- source ``rtp://``, repair none (bare RTP without FEC)
- source ``rtp+rs8m://``, repair ``rs8m://`` (RTP with Reed-Solomon FEC)
- source ``rtp+ldpc://``, repair ``ldpc://`` (RTP with LDPC-Staircase FEC)

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

Broadcast address
-----------------

This tool follows the common convention is to forbid traffic to broadcast addresses unless allowed excplicitly, to prevent accidental flood. To allow sending packets to broadcast source or repair endpoints, use ``--broadcast`` option.

Time units
----------

*TIME* should have one of the following forms:
  123ns, 123us, 123ms, 123s, 123m, 123h

EXAMPLES
========

Endpoint examples
-----------------

Send file to one bare RTP endpoint:

.. code::

    $ roc-send -vv -i file:./input.wav -s rtp://192.168.0.3:10001

Send file to two IPv4 source and repair endpoints:

.. code::

    $ roc-send -vv -i file:./input.wav -s rtp+rs8m://192.168.0.3:10001 -r rs8m://192.168.0.3:10002

Send file to two IPv6 source and repair endpoints:

.. code::

    $ roc-send -vv -i file:./input.wav -s rtp+rs8m://[2001:db8::]:10001 -r rs8m://[2001:db8::]:10002

Send file to two broadcast endpoints:

.. code::

    $ roc-send -vv -i file:./input.wav -s rtp+rs8m://192.168.0.3:10001 -r rs8m://192.168.0.3:10002 --broadcast

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

Send WAV file, specify format manually:

.. code::

    $ roc-send -vv -s rtp://192.168.0.3:10001 -i file:./input --input-format wav

Send WAV from stdin:

.. code::

    $ roc-send -vv -s rtp://192.168.0.3:10001 -i file:- --input-format wav <./input.wav

Send WAV file, specify full URI:

.. code::

    $ roc-send -vv -s rtp://192.168.0.3:10001 -i file:///home/user/input.wav

Tuning examples
---------------

Force a specific rate on the input device:

.. code::

    $ roc-send -vv -s rtp://192.168.0.3:10001 --rate=44100

Select the LDPC-Staircase FEC scheme and a larger block size:

.. code::

    $ roc-send -vv -i file:./input.wav \
        -s rtp+ldpc://192.168.0.3:10001 -r ldpc://192.168.0.3:10002 \
        --nbsrc=1000 --nbrpr=500

Select resampler profile:

.. code::

    $ roc-send -vv -s rtp://192.168.0.3:10001 --resampler-profile=high

SEE ALSO
========

:manpage:`roc-recv(1)`, and the Roc web site at https://roc-project.github.io/

BUGS
====

Please report any bugs found via GitHub (https://github.com/roc-project/roc/).

AUTHORS
=======

See `authors <https://roc-project.github.io/roc/docs/about_project/authors.html>`_ page on the website for a list of maintainers and contributors.
