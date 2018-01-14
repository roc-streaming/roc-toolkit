roc-send
********

SYNOPSIS
========

**roc-send** *[OPTIONS]* *ADDRESS*

DESCRIPTION
===========

Send audio-stream from a file or audio device to a Roc receiver.

Options
-------

-h, --help             Print help and exit
-V, --version          Print version and exit
-v, --verbose          Increase verbosity level (may be used multiple times)
-i, --input=NAME       Input file or device
-t, --type=TYPE        Input codec or driver
-s, --source=ADDRESS   Remote source UDP address
-r, --repair=ADDRESS   Remote repair UDP address
-l, --local=ADDRESS    Local UDP address
--fec=ENUM             FEC scheme  (possible values="rs", "ldpc", "none" default="rs")
--nbsrc=INT            Number of source packets in FEC block
--nbrpr=INT            Number of repair packets in FEC block
--interleaving=ENUM    Enable/disable packet interleaving  (possible values="yes", "no" default="no")
--rate=INT             Sample rate (Hz)

Address
-------

*ADDRESS* should be in one of the following forms:

- :PORT (e.g. ":12345")
- IPv4:PORT (e.g. "127.0.0.1:12345")
- [IPv6]:PORT (e.g. "[::1]:12345")

Input
-----

Arguments for ``--input`` and ``--type`` options are passed to SoX:

- *NAME* specifies file or device name
- *TYPE* specifies file or device type

EXAMPLES
========

Send wav file:

.. code::

    $ roc-send -vv -s 192.168.0.3:12345 -r 192.168.0.3:12346 -i song.wav

Or:

.. code::

    $ roc-send -vv -s 192.168.0.3:12345 -r 192.168.0.3:12346 -i song.wav -t wav

Capture sound from default driver and device:

.. code::

    $ roc-send -vv -s 192.168.0.3:12345 -r 192.168.0.3:12346

Capture sound from default ALSA device:

.. code::

    $ roc-send -vv -s 192.168.0.3:12345 -r 192.168.0.3:12346 -t alsa

Or:

.. code::

    $ roc-send -vv -s 192.168.0.3:12345 -r 192.168.0.3:12346 -t alsa -i default

Capture sound from specific pulseaudio device:

.. code::

    $ roc-send -vv -s 192.168.0.3:12345 -r 192.168.0.3:12346 -t pulseaudio -i <device>

SEE ALSO
========

:manpage:`roc-recv(1)`, :manpage:`sox(1)`, the Roc web site at https://roc-project.github.io/

BUGS
====

Please report any bugs found via GitHub issues (https://github.com/roc-project/roc/).

AUTHORS
=======

See the AUTHORS file for a list of maintainers and contributors.
