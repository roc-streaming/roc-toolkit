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

-i, --input=FILE_URI           Input file URI
--input-format=FILE_FORMAT     Force input file format
-o, --output=FILE_URI          Output file URI
--output-format=FILE_FORMAT    Force output file format
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

File URI
--------

``--input`` and ``--output`` options define input / output file URI.

*FILE_URI* should have one of the following forms:

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

The ``--input-format`` and ``--output-format`` options can be used to force the file format. If the option is omitted, the file format is auto-detected. This option is always required for stdin or stdout.

The path component of the provided URI is `percent-decoded <https://en.wikipedia.org/wiki/Percent-encoding>`_. For convenience, unencoded characters are allowed as well, except that ``%`` should be always encoded as ``%25``.

For example, the file named ``/foo/bar%/[baz]`` may be specified using either of the following URIs: ``file:///foo%2Fbar%25%2F%5Bbaz%5D`` and ``file:///foo/bar%25/[baz]``.

Time units
----------

*TIME* defines duration with nanosecond precision.

It should have one of the following forms:
  123ns; 1.23us; 1.23ms; 1.23s; 1.23m; 1.23h;

EXAMPLES
========

Convert sample rate to 24-bit 48k stereo:

.. code::

    $ roc-copy -vv --io-encoding s24/48000/stereo -i file:input.wav -o file:output.wav

Same, but drop output results instead of writing to file (useful for benchmarking):

.. code::

    $ roc-copy -vv --io-encoding s24/48000/stereo -i file:input.wav

Input from stdin, output to stdout:

.. code::

    $ roc-copy -vv --input-format=wav -i file:- \
        --output-format=wav -o file:- >./output.wav <./input.wav

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
