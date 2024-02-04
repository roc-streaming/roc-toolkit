roc-copy
********

SYNOPSIS
========

**roc-copy** *OPTIONS*

DESCRIPTION
===========

Read audio stream from a file, transform it, and and write it to a another file.

Options
-------

-h, --help                   Print help and exit
-V, --version                Print version and exit
-v, --verbose                Increase verbosity level (may be used multiple times)
-L, --list-supported         list supported schemes and formats
-i, --input=FILE_URI         Input file URI
-o, --output=FILE_URI        Output file URI
--input-format=FILE_FORMAT   Force input file format
--output-format=FILE_FORMAT  Force output file format
--frame-len=TIME             Duration of the internal frames, TIME units
-r, --rate=INT               Output sample rate, Hz
--resampler-backend=ENUM     Resampler backend  (possible values="default", "builtin", "speex", "speexdec" default=`default')
--resampler-profile=ENUM     Resampler profile  (possible values="low", "medium", "high" default=`medium')
--profiling                  Enable self profiling  (default=off)
--color=ENUM                 Set colored logging mode for stderr output (possible values="auto", "always", "never" default=`auto')

File URI
--------

``--input`` and ``--output`` options require a file URI in one of the following forms:

- ``file:///ABS/PATH`` -- absolute file path
- ``file://localhost/ABS/PATH`` -- absolute file path (alternative form; only "localhost" host is supported)
- ``file:/ABS/PATH`` -- absolute file path (alternative form)
- ``file:REL/PATH`` -- relative file path
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

*TIME* should have one of the following forms:
  123ns; 1.23us; 1.23ms; 1.23s; 1.23m; 1.23h;

EXAMPLES
========

Convert sample rate to 48k:

.. code::

    $ roc-copy -vv --rate=48000 -i file:input.wav -o file:output.wav

Drop output results (useful for benchmarking):

.. code::

    $ roc-copy -vv --rate=48000 -i file:input.wav

Input from stdin, output to stdout:

.. code::

    $ roc-copy -vv --input-format=wav -i file:- \
        --output-format=wav -o file:- >./output.wav <./input.wav

ENVIRONMENT VARIABLES
=====================

The following environment variables are supported:

NO_COLOR
    By default, terminal coloring is automatically detected. This environment variable can be set to a non-empty string to disable terminal coloring. It has lower precedence than ``--color`` option.

FORCE_COLOR
    By default, terminal coloring is automatically detected. This environment variable can be set to a positive integer to enable/force terminal coloring. It has lower precedence than  ``NO_COLOR`` variable and ``--color`` option.

SEE ALSO
========

:manpage:`roc-recv(1)`, :manpage:`roc-send(1)`, the Roc web site at https://roc-streaming.org/

BUGS
====

Please report any bugs found via GitHub (https://github.com/roc-streaming/roc-toolkit/).

AUTHORS
=======

See authors page on the website for a list of maintainers and contributors (https://roc-streaming.org/toolkit/docs/about_project/authors.html).
