roc-recv
********

SYNOPSIS
========

**roc-recv** *OPTIONS*

DESCRIPTION
===========

Receive audio-stream on particular UDP port(s), repair lost packets, adjust sampling frequincy to the sender, and write it to a file or audio device.

Options
-------

-h, --help                Print help and exit
-V, --version             Print version and exit
-v, --verbose             Increase verbosity level (may be used multiple times)
-o, --output=NAME         Output file or device
-t, --type=TYPE           Output codec or driver
-s, --source=ADDRESS      Source UDP address
-r, --repair=ADDRESS      Repair UDP address
--fec=ENUM                FEC scheme  (possible values="rs", "ldpc", "none" default=`rs')
--nbsrc=INT               Number of source packets in FEC block
--nbrpr=INT               Number of repair packets in FEC block
--silence-timeout=INT     Session timeout for silence, number of samples
--drops-timeout=INT       Session timeout for constant drops, number of samples
--drops-window=INT        Session drops detection window, number of samples
--latency=INT             Session target latency, number of samples
--min-latency=INT         Session minimum latency, number of samples
--max-latency=INT         Session maximum latency, number of samples
--rate=INT                Override output sample rate (Hz)
--no-resampling           Disable resampling  (default=off)
--resampler-profile=ENUM  Resampler profile  (possible values="low", "medium", "high" default=`medium')
--resampler-interp=INT    Resampler sinc table precision
--resampler-window=INT    Number of samples per resampler window
-1, --oneshot                 Exit when last connected client disconnects (default=off)
--poisoning               Enable uninitialized memory poisoning (default=off)
--beeping                 Enable beeping on packet loss  (default=off)

Address
-------

*ADDRESS* should be in one of the following forms:

- :PORT (e.g. ":12345")
- IPv4:PORT (e.g. "127.0.0.1:12345")
- [IPv6]:PORT (e.g. "[::1]:12345")

Output
------

Arguments for ``--output`` and ``--type`` options are passed to SoX:

- *NAME* specifies file or device name
- *TYPE* specifies file or device type

EXAMPLES
========

Start receiver listening on all interfaces on two UDP ports:

.. code::

    $ roc-recv -vv -s :12345 -r :12346

Start receiver listening on particular interface:

.. code::

    $ roc-recv -vv -s 192.168.0.3:12345 -r 192.168.0.3:12346

Output to ALSA default device:

.. code::

    $ roc-recv -vv -s :12345 -r :12346 -t alsa

Or:

.. code::

    $ roc-recv -vv -s :12345 -r :12346 -t alsa -o default

Output to a file:

.. code::

    $ roc-recv -vv -s :12345 -r :12346 -o record.wav

Or:

.. code::

    $ roc-recv -vv -s :12345 -r :12346 -o record.wav -t wav

SEE ALSO
========

:manpage:`roc-send(1)`, :manpage:`roc-conv(1)`, :manpage:`sox(1)`, the Roc web site at https://roc-project.github.io/

BUGS
====

Please report any bugs found via GitHub issues (https://github.com/roc-project/roc/).

AUTHORS
=======

See the AUTHORS file for a list of maintainers and contributors.
