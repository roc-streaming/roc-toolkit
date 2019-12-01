roc-conv
********

SYNOPSIS
========

**roc-conv** *OPTIONS*

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
--frame-length=TIME          Duration of the internal frames, TIME units
-r, --rate=INT               Output sample rate, Hz
--no-resampling              Disable resampling  (default=off)
--resampler-backend=ENUM     Resampler backend  (possible values="builtin" default=`builtin')
--resampler-profile=ENUM     Resampler profile  (possible values="low", "medium", "high" default=`medium')
--resampler-interp=INT       Resampler sinc table precision
--resampler-window=INT       Number of samples per resampler window
--poisoning                  Enable uninitialized memory poisoning (default=off)
--profiling                  Enable self profiling (default=off)
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

EXAMPLES
========

Convert sample rate to 48k:

.. code::

    $ roc-conv -vv --rate=48000 -i file:input.wav -o file:output.wav

Drop output results (useful for benchmarking):

.. code::

    $ roc-conv -vv --rate=48000 -i file:input.wav

Input from stdin, output to stdout:

.. code::

    $ roc-conv -vv --input-format=wav -i file:- --output-format=wav -o file:- >./output.wav <./input.wav

SEE ALSO
========

:manpage:`roc-recv(1)`, :manpage:`roc-send(1)`, the Roc web site at https://roc-project.github.io/

BUGS
====

Please report any bugs found via GitHub (https://github.com/roc-project/roc/).

AUTHORS
=======

See `authors <https://roc-project.github.io/roc/docs/about_project/authors.html>`_ page on the website for a list of maintainers and contributors.
