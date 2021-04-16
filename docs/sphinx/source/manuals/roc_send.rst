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
-i, --input=INPUT         Input file or device
-d, --driver=DRIVER       Input driver
-s, --source=PORT         Remote source port triplet
-r, --repair=PORT         Remote repair port triplet
--nbsrc=INT               Number of source packets in FEC block
--nbrpr=INT               Number of repair packets in FEC block
--packet-length=STRING    Outgoing packet length, TIME units
--packet-limit=INT        Maximum packet size, in bytes
--frame-size=INT          Internal frame size, number of samples
--rate=INT                Override input sample rate, Hz
--no-resampling           Disable resampling  (default=off)
--resampler-profile=ENUM  Resampler profile  (possible values="low", "medium", "high" default=`medium')
--resampler-interp=INT    Resampler sinc table precision
--resampler-window=INT    Number of samples per resampler window
--interleaving            Enable packet interleaving  (default=off)
--poisoning               Enable uninitialized memory poisoning (default=off)

Input
-----

*INPUT* should be file name or device name, for example:

- \- (stdin)
- file.wav (WAV file)
- default (default input device)
- front:CARD=PCH,DEV=0 (ALSA device)
- alsa_input.pci-0000_00_1f.3.analog-stereo (PulseAudio source)

Interpretation of the device name depends on the selected driver.

If the input is omitted, some default input is selected. If the driver is omitted or it is a device driver, the default input device is seelcted. If the driver is a file driver, the stdin is selected.

Driver
------

*DRIVER* defines the type of the input file or device, for example:

- wav
- alsa
- pulseaudio

If the driver is omitted, some default driver is selected. If the user did specify the input and it is a file with a known extension, the appropriate file driver is selected. Otherwise, the first device driver available on the system is selected.

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

Time
----

*TIME* should have one of the following forms:
  123ns, 123us, 123ms, 123s, 123m, 123h

EXAMPLES
========

Send WAV file:

.. code::

    $ roc-send -vv -s rtp+rs8m:192.168.0.3:10001 -r rs8m:192.168.0.3:10002 -i ./file.wav

Send WAV file to an IPv6 receiver:

.. code::

    $ roc-send -vv -s rtp+rs8m:[2001:db8::]:10001 -r rs8m:[2001:db8::]:10002 -i ./file.wav

Send WAV from stdin:

.. code::

    $ roc-send -vv -s rtp+rs8m:192.168.0.3:10001 -r rs8m:192.168.0.3:10002 -d wav -i - < ./file.wav

Capture sound from the default driver and device:

.. code::

    $ roc-send -vv -s rtp+rs8m:192.168.0.3:10001 -r rs8m:192.168.0.3:10002

Capture sound from the default ALSA device:

.. code::

    $ roc-send -vv -s rtp+rs8m:192.168.0.3:10001 -r rs8m:192.168.0.3:10002 -d alsa

Capture sound from a specific PulseAudio device:

.. code::

    $ roc-send -vv -s rtp+rs8m:192.168.0.3:10001 -r rs8m:192.168.0.3:10002 -d pulseaudio -i <device>

Force a specific rate on the input device:

.. code::

    $ roc-send -vv -s rtp+rs8m:192.168.0.3:10001 -r rs8m:192.168.0.3:10002 --rate=44100

Select the LDPC-Staircase FEC scheme and a larger block size:

.. code::

    $ roc-send -vv -s rtp+ldpc:192.168.0.3:10003 -r ldpc:192.168.0.3:10004 -i ./file.wav \
      --nbsrc=1000 --nbrpr=500

Select bare RTP without FEC:

.. code::

    $ roc-send -vv -s rtp:192.168.0.3:10005 -i ./file.wav

Select resampler profile:

.. code::

    $ roc-send -vv -s rtp+rs8m:192.168.0.3:10001 -r rs8m:192.168.0.3:10002 --resampler-profile=high

SEE ALSO
========

:manpage:`roc-recv(1)`, :manpage:`roc-conv(1)`, :manpage:`sox(1)`, the Roc web site at https://roc-streaming.org/

BUGS
====

Please report any bugs found via GitHub (https://github.com/roc-streaming/roc-toolkit/).

AUTHORS
=======

See `authors <https://roc-streaming.org/toolkit/docs/about_project/authors.html>`_ page on the website for a list of maintainers and contributors.
