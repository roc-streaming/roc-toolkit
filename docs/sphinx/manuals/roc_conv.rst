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
-r, --rate=INT            Output sample rate (Hz)
--frame-size=INT          Number of samples per audio frame
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

:manpage:`roc-recv(1)`, :manpage:`roc-send(1)`, the Roc web site at https://roc-project.github.io/

BUGS
====

Please report any bugs found via GitHub issues (https://github.com/roc-project/roc/).

AUTHORS
=======

See the AUTHORS file for a list of maintainers and contributors.
