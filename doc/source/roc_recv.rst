.. _roc_recv:

roc-recv
********

SYNOPSIS
========

**roc-recv** [-v|--verbose] [-o|--output=NAME] [-t|--type=TYPE] [--fec=ENUM] [--nbsrc=INT] [--nbrpr=INT] [--resampling=ENUM] [--timing=ENUM] [-1|--oneshot] [--beep] [--rate=INT] [--session-timeout=INT] [--session-latency=INT] [--output-latency=INT] [--output-frame=INT] [--resampler-frame=INT] ADDRESS

DESCRIPTION
===========

Roc provides two command-line utilities for transmitting audio-content via LAN/WLAN. They're pretty simple but very useful.

Roc-recv receives audio-stream on a particular UDP port, repairs lost packets, adjusts sampling frequincy to the sender, and plays content with SoX or ALSA.

OPTIONS
=======

-v, --verbose         
	Increase verbosity level (may be used multiple times)

-o, --output=NAME     
	Output file or device

-t, --type=TYPE       
	Output codec or driver

--fec=ENUM            
	FEC scheme  (possible values="rs", "ldpc", "none" default=`rs')

--nbsrc=INT           
	Number of source packets in FEC block

--nbrpr=INT           
	Number of repair packets in FEC block

--resampling=ENUM     
	Enabled/disable resampling  (possible values="yes", "no" default=`yes')

--timing=ENUM         
	Enabled/disable pipeline timing  (possible values="yes", "no" default=`yes')

-1, --oneshot         
	Exit when last connected client disconnects (default=off)

--beep                
	Enable beep on packet loss  (default=off)

--rate=INT            
	Sample rate (Hz)

--session-timeout=INT 
	Session timeout as number of samples

--session-latency=INT 
	Session latency as number of samples

--output-latency=INT  
	Output latency as number of samples

--output-frame=INT    
	Number of samples per output frame

--resampler-frame=INT 
	Number of samples per resampler frame

ADDRESS should be in form of [IP]:PORT. IP defaults to 0.0.0.0.

OUTPUT
======

Arguments for '--output' and '--type' options are passed to SoX:

* NAME specifies file or device name

* TYPE specifies file or device type

EXAMPLES
========

Start receiver listening on all interfaces on UDP port 12345:

    ``$ roc-recv -vv :12345``

Start receiver listening on particular interface:

    ``$ roc-recv -vv 192.168.0.3:12345``

Output to ALSA default device:

    ``$ roc-recv -vv :12345 -t alsa``

or

    ``$ roc-recv -vv :12345 -t alsa -o default``

Output to file:

	``$ roc-recv -vv :12345 -o record.wav``

or

    ``$ roc-recv -vv :12345 -o record.wav -t wav``

