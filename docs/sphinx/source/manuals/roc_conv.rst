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

-h, --help                Print help and exit
-V, --version             Print version and exit
-v, --verbose             Increase verbosity level (may be used multiple times)
-i, --input=PATH          Input file
-o, --output=PATH         Output file
--frame-size=INT          Internal frame size, number of samples
-r, --rate=INT            Output sample rate, Hz
--no-resampling           Disable resampling  (default=off)
--resampler-profile=ENUM  Resampler profile  (possible values="low", "medium", "high" default=`medium')
--resampler-interp=INT    Resampler sinc table precision
--resampler-window=INT    Number of samples per resampler window
--poisoning               Enable uninitialized memory poisoning (default=off)

EXAMPLES
========

Convert sample rate to 48k:

.. code::

    $ roc-conv -vv -r 48000 -i input.wav -o output.wav

SEE ALSO
========

:manpage:`roc-recv(1)`, :manpage:`roc-send(1)`, the Roc web site at https://roc-streaming.org/

BUGS
====

Please report any bugs found via GitHub (https://github.com/roc-streaming/roc-toolkit/).

AUTHORS
=======

See `authors <https://roc-streaming.org/toolkit/docs/about_project/authors.html>`_ page on the website for a list of maintainers and contributors.
