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
-v, --verbose                 Increase verbosity level (may be used multiple times)
-o, --output=NAME             Output file or device
-t, --type=TYPE               Output codec or driver
-s, --source=ADDRESS          Source UDP address
-r, --repair=ADDRESS          Repair UDP address
--fec=ENUM                    FEC scheme  (possible values="rs", "ldpc", "none" default=`rs')
--nbsrc=INT                   Number of source packets in FEC block
--nbrpr=INT                   Number of repair packets in FEC block
--latency=STRING              Session target latency, TIME units
--min-latency=STRING          Session minimum latency, TIME units
--max-latency=STRING          Session maximum latency, TIME units
--np-timeout=STRING           Session no playback timeout, TIME units
--bp-timeout=STRING           Session broken playback timeout, TIME units
--bp-window=STRING            Session breakage detection window, TIME units
--rate=INT                    Override output sample rate, Hz
--no-resampling               Disable resampling  (default=off)
--resampler-profile=ENUM      Resampler profile  (possible values="low", "medium", "high" default=`medium')
--resampler-interp=INT        Resampler sinc table precision
--resampler-window=INT        Number of samples per resampler window
-1, --oneshot                 Exit when last connected client disconnects (default=off)
--poisoning                   Enable uninitialized memory poisoning (default=off)
--beeping                     Enable beeping on packet loss  (default=off)

Address
-------

*ADDRESS* should be in one of the following forms:

- :PORT (e.g. ":10001")
- IPv4:PORT (e.g. "127.0.0.1:10001")
- [IPv6]:PORT (e.g. "[::1]:10001")

Output
------

Arguments for ``--output`` and ``--type`` options are passed to SoX:

- *NAME* specifies file or device name
- *TYPE* specifies file or device type

Time
----

*TIME* should have one of the following forms:
  123ns, 123us, 123ms, 123s, 123m, 123h

EXAMPLES
========

Start receiver listening on all interfaces on two UDP ports:

.. code::

    $ roc-recv -vv -s :10001 -r :10002

Start receiver listening on particular interface:

.. code::

    $ roc-recv -vv -s 192.168.0.3:10001 -r 192.168.0.3:10002

Output to the default ALSA device:

.. code::

    $ roc-recv -vv -s :10001 -r :10002 -t alsa

Output to a specific PulseAudio device:

.. code::

    $ roc-recv -vv -s :10001 -r :10002 -t pulseaudio -o <device>

Output to a file in WAV format:

.. code::

    $ roc-recv -vv -s :10001 -r :10002 -o ./file.wav

Output to stdout in WAV format:

.. code::

    $ roc-recv -vv -s :10001 -r :10002 -t wav -o - > ./file.wav

Select higher latency and timeouts:

.. code::

    $ roc-recv -vv -s :10001 -r :10002 \
      --latency=5s --min-latency=-1s --max-latency=10s --np-timeout=10s --bp-timeout=10s

Force a specific output rate to be requested on the audio device:

.. code::

    $ roc-recv -vv -s :10001 -r :10002 --rate=44100

Select resampler profile:

.. code::

    $ roc-recv -vv -s :10001 -r :10002 --resampler-profile=high

SEE ALSO
========

:manpage:`roc-send(1)`, :manpage:`roc-conv(1)`, :manpage:`sox(1)`, the Roc web site at https://roc-project.github.io/

BUGS
====

Please report any bugs found via GitHub issues (https://github.com/roc-project/roc/).

AUTHORS
=======

See the AUTHORS file for a list of maintainers and contributors.
