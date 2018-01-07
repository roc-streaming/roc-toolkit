.. _roc_send:

roc-send
********

SYNOPSIS
========

**roc-send** [-v|--verbose] [-s|--source=ADRESS] [-i|--input=NAME] [-t|--type=TYPE] [--fec=ENUM] [--nbsrc=N] [--nbrpr=N] [--interleaving=ENUM] [--timing=ENUM] [--rate=N] [--loss-rate=N] [--delay-rate=N] [--delay=N] ADDRESS

DESCRIPTION
===========

Roc provides two command-line utilities for transmitting audio-content via LAN/WLAN. They're pretty simple but very useful.

Roc-send sends audio-stream from a file to a roc receiver (e.g. roc-recv). 

OPTIONS
=======

-v, --verbose
	Increase verbosity level (may be used multiple times)

-s, --source=ADRESS
    Source address (default is 0.0.0.0:0, i.e. INADDR_ANY and random port)

-i, --input=NAME
    Input file or device

-t, --type=TYPE
    Input codec or driver

--fec=ENUM
	FEC scheme  (possible values="rs", "ldpc", "none" default=`rs')

--nbsrc=INT
    Number of source packets in FEC block

--nbrpr=INT
    Number of repair packets in FEC block

--interleaving=ENUM
	Enable/disable packet interleaving  (possible values="yes", "no" default=`yes')

--timing=ENUM
	Enable/disable pipeline timing  (possible values="yes", "no" default=`yes')

--rate=INT
	Sample rate (Hz)

--loss-rate=INT
    Set percentage of packets to be randomly lost, [0; 100]

--delay-rate=INT
	Set percentage of packets to be randomly delayed, [0; 100]

--delay=INT
	Set delay time, milliseconds

ADDRESS should be in form of [IP]:PORT. IP defaults to 0.0.0.0.

INPUT
======

Arguments for '--input' and '--type' options are passed to SoX:

* NAME specifies file or device name

* TYPE specifies file or device type

EXAMPLES
========

Send wav file:

	``$ roc-send -vv 192.168.0.3:12345 -i song.wav``

or

	``$ roc-send -vv 192.168.0.3:12345 -i song.wav -t wav``

Capture sound from default driver and device:

	``$ roc-send -vv 192.168.0.3:12345``

Capture sound from default ALSA device:

	``$ roc-send -vv 192.168.0.3:12345 -t alsa``

or

	``$ roc-send -vv 192.168.0.3:12345 -t alsa -i default``

Capture sound from specific pulseaudio device:

	``$ roc-send -vv 192.168.0.3:12345 -t pulseaudio -i <device>``
