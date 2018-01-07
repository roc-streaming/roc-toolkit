Flow
****

.. warning::

   This section is under construction.

Sender timing
=============

One of the valuable feature of roc is that a receiver (e.g. **roc-recv**) adjusts its sampling frequency to a sampling rate of a sender. That's the reason why a sender must send packets with steady rate. In other words it's forbidden to send an entire .wav file at once. There're many other ways for that.

As packets must be scheduled carefully, roc could do this job for sender. Unless a user turns on ``ROC_API_CONF_DISABLE_TIMING`` option in ``roc_config::options``, ``roc_sender_write`` will block sender's thread until the samples are sent. This variant could be used in e.g. media player, when there's no other source of timing.

Otherwise, ``roc_sender_write`` stores samples in a queue and returns immediately which could be usefull when sender's thread blocks on acquiring the samples, e.g. in `the module for pulseaudio <https://github.com/roc-project/pulseaudio-roc>`_.

The example didn't disable timing because it generates samples online, so it needs to be blocked so as not to oveflow roc's senders queue.
