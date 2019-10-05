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
-o, --output=OUTPUT       Output file or device
-d, --driver=DRIVER       Output driver
-s, --source=PORT         Source port triplet
-r, --repair=PORT         Repair port triplet
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

Output
------

*OUTPUT* should be file name or device name, for example:

- \- (stdout)
- file.wav (WAV file)
- default (default output device)
- front:CARD=PCH,DEV=0 (ALSA device)
- alsa_output.pci-0000_00_1f.3.analog-stereo (PulseAudio sink)

Interpretation of the device name depends on the selected driver.

If the output is omitted, some default output is selected. If the driver is omitted or it is a device driver, the default output device is seelcted. If the driver is a file driver, the stdout is selected.

Driver
------

*DRIVER* defines the type of the output file or device, for example:

- wav
- alsa
- pulseaudio

If the driver is omitted, some default driver is selected. If the user did specify the output and it is a file with a known extension, the appropriate file driver is selected. Otherwise, the first device driver available on the system is selected.

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

Time
----

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

Output to the default ALSA device:

.. code::

    $ roc-recv -vv -s rtp+rs8m::10001 -r rs8m::10002 -d alsa

Output to a specific PulseAudio device:

.. code::

    $ roc-recv -vv -s rtp+rs8m::10001 -r rs8m::10002 -d pulseaudio -o <device>

Output to a file in WAV format:

.. code::

    $ roc-recv -vv -s rtp+rs8m::10001 -r rs8m::10002 -o ./file.wav

Output to stdout in WAV format:

.. code::

    $ roc-recv -vv -s rtp+rs8m::10001 -r rs8m::10002 -d wav -o - > ./file.wav

Force a specific rate on the output device:

.. code::

    $ roc-recv -vv -s rtp+rs8m::10001 -r rs8m::10002 --rate=44100

Select higher session latency and timeouts:

.. code::

    $ roc-recv -vv -s rtp+rs8m::10001 -r rs8m::10002 \
      --sess-latency=5s --min-latency=-1s --max-latency=10s --np-timeout=10s --bp-timeout=10s

Select higher I/O latency:

.. code::

    $ roc-recv -vv -s rtp+rs8m::10001 -r rs8m::10002 --io-latency=200ms

Select resampler profile:

.. code::

    $ roc-recv -vv -s rtp+rs8m::10001 -r rs8m::10002 --resampler-profile=high

SEE ALSO
========

:manpage:`roc-send(1)`, :manpage:`roc-conv(1)`, :manpage:`sox(1)`, the Roc web site at https://roc-project.github.io/

BUGS
====

Please report any bugs found via GitHub (https://github.com/roc-project/roc/).

AUTHORS
=======

See `authors <https://roc-project.github.io/roc/docs/about_project/authors.html>`_ page on the website for a list of maintainers and contributors.
