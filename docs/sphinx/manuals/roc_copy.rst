roc-copy
********

.. only:: html

  .. contents:: Table of contents:
     :local:
     :depth: 1

SYNOPSIS
========

**roc-copy** *OPTIONS*

DESCRIPTION
===========

Read audio stream from a file, transcode, and write to a another file.

.. begin_options

General options
---------------

-h, --help            Print help and exit
-V, --version         Print version and exit
-v, --verbose         Increase verbosity level (may be used multiple times)
--color=ENUM          Set colored logging mode for stderr output (possible values="auto", "always", "never" default=`auto')
-L, --list-supported  List supported protocols, formats, etc.

I/O options
-----------

-i, --input=IO_URI             Input file URI
--input-encoding=IO_ENCODING   Input file encoding
-o, --output=IO_URI            Output file URI
--output-encoding=IO_ENCODING  Output file encoding
--io-frame-len=TIME            I/O frame length, TIME units

Transcoding options
-------------------

--resampler-backend=ENUM  Resampler backend  (possible values="default", "builtin", "speex", "speexdec" default=`default')
--resampler-profile=ENUM  Resampler profile  (possible values="low", "medium", "high" default=`medium')

Debugging options
-----------------

--prof  Enable self-profiling  (default=off)

.. end_options

DETAILS
=======

I/O URI
-------

``--input`` and ``--output`` options define input / output file URI.

*IO_URI* should have one of the following forms:

- ``file:///<abs>/<path>`` -- absolute file path
- ``file://localhost/<abs>/<path>`` -- absolute file path (alternative form; only "localhost" host is supported)
- ``file:/<abs>/<path>`` -- absolute file path (alternative form)
- ``file:<rel>/<path>`` -- relative file path
- ``file://-`` -- stdout
- ``file:-`` -- stdout (alternative form)

Examples:

- ``file:///home/user/test.wav``
- ``file://localhost/home/user/test.wav``
- ``file:/home/user/test.wav``
- ``file:./test.wav``
- ``file:-``

The list of supported file formats can be retrieved using ``--list-supported`` option.

If the ``--output`` is omitted, the conversion results are discarded.

The path component of the provided URI is `percent-decoded <https://en.wikipedia.org/wiki/Percent-encoding>`_. For convenience, unencoded characters are allowed as well, except that ``%`` should be always encoded as ``%25``.

For example, the file named ``/foo/bar%/[baz]`` may be specified using either of the following URIs: ``file:///foo%2Fbar%25%2F%5Bbaz%5D`` and ``file:///foo/bar%25/[baz]``.

I/O encoding
------------

``--input-encoding`` and ``--output-encoding`` options allow to explicitly specify encoding of the input or output file.

This option is useful when file encoding can't be detected automatically (e.g. file doesn't have extension or uses header-less format like raw PCM).

*IO_ENCODING* should have the following form:

``<format>[@<subformat>]/<rate>/<channels>``

Where:

* ``format`` defines container format, e.g. ``pcm`` (raw samples), ``wav``, ``ogg``
* ``subformat`` is optional format-dependent codec, e.g. ``s16`` for ``pcm`` or ``wav``, and ``vorbis`` for ``ogg``
* ``rate`` defines sample rate in Hertz (number of samples per second), e.g. ``48000``
* ``channels`` defines channel layout, e.g. ``mono`` or ``stereo``

``format``, ``rate``, and ``channels`` may be set to special value ``-``, which means using default value for input device, or auto-detect value for input file.

Whether ``subformat`` is required, allowed, and what values are accepted, depends on ``format``.

Examples:

* ``pcm@s16/44100/mono`` -- PCM, 16-bit native-endian integers, 44.1KHz, 1 channel
* ``pcm@f32_le/48000/stereo`` -- PCM, 32-bit little-endian floats, 48KHz, 2 channels
* ``wav/-/-`` -- WAV file, auto-detect sub-format, rate, channels
* ``flac-/-/-`` -- FLAC file, auto-detect sub-format, rate, channels

The list of supported formats, sub-formats, and channel layouts can be retrieved using ``--list-supported`` option.

Time units
----------

*TIME* defines duration with nanosecond precision.

It should have one of the following forms:
  123ns; 1.23us; 1.23ms; 1.23s; 1.23m; 1.23h;

EXAMPLES
========

Convert sample rate to 24-bit 48k stereo:

.. code::

    $ roc-copy -vv -i file:input.wav -o file:output.wav --output-encoding wav@s24/48000/stereo

Same, but drop output results instead of writing to file (useful for benchmarking):

.. code::

    $ roc-copy -vv -i file:input.wav --output-encoding pcm@s24/48000/stereo

Input from stdin, output to stdout:

.. code::

    $ roc-copy -vv --input-encoding=wav/-/- -i file:- \
        --output-encoding=wav/-/- -o file:- >./output.wav <./input.wav

ENVIRONMENT
===========

The following environment variables are supported:

NO_COLOR
    By default, terminal coloring is automatically detected. This environment variable can be set to a non-empty string to disable terminal coloring. It has lower precedence than ``--color`` option.

FORCE_COLOR
    By default, terminal coloring is automatically detected. This environment variable can be set to a positive integer to enable/force terminal coloring. It has lower precedence than  ``NO_COLOR`` variable and ``--color`` option.

SEE ALSO
========

:manpage:`roc-send(1)`, :manpage:`roc-recv(1)`, and the Roc web site at https://roc-streaming.org/

BUGS
====

Please report any bugs found via GitHub (https://github.com/roc-streaming/roc-toolkit/).

AUTHORS
=======

See authors page on the website for a list of maintainers and contributors (https://roc-streaming.org/toolkit/docs/about_project/authors.html).
