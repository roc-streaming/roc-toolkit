Data flow
*********

.. warning::

   This section is under construction.

Pipelines
=========

Roc's design is all about pipelines of connected *readers* or *writers*. This pattern is repeated on several levels:

* when processing network datagrams;
* when processing higher-level packets;
* when processing audio samples.

Pipeline begins with a *producer* or a *queue*, and ends with a *consumer* or a *queue*. Producer and consumer are connected through a sequence of readers or writers, depending on pipeline direction.

Threads
=======

There are at most three threads, communicating strictly through thread-safe queues:

* datagram queue;
* sample buffer queue.

In sender:
----------

================= ================= =================
Thread	          Module	        Description
================= ================= =================
audio reader	  ``roc_sndio``	        Reads audio stream from input file or device and writes it to sample buffer queue.
sender pipeline	  ``roc_pipeline``	    Reads sample buffers from queue, packs them into packets, composes datagrams from packets, and writes them to datagram queue.
network sender	  ``roc_netio``	        Reads datagrams from queue and sends them to receiver.
================= ================= =================

In receiver:
------------

================= ================= =================
Thread	          Module	        Description
================= ================= =================
network receive   ``roc_netio``     Receives datagrams and writes them to datagram queue.
receiver pipeline ``roc_pipeline``  Reads datagrams from queue, parses packets from datagrams, reconstructs audio stream from packets, and writes it to sample buffer queue.
audio writer      ``roc_sndio``     Reads sample buffers from queue and sends them to output file or device.
================= ================= =================

Sender timing
=============

One of the valuable feature of roc is that a receiver (e.g. **roc-recv**) adjusts its sampling frequency to a sampling rate of a sender. That's the reason why a sender must send packets with steady rate. In other words it's forbidden to send an entire .wav file at once. There're many other ways for that.

As packets must be scheduled carefully, roc could do this job for sender. Unless a user turns on ``ROC_API_CONF_DISABLE_TIMING`` option in ``roc_config::options``, ``roc_sender_write`` will block sender's thread until the samples are sent. This variant could be used in e.g. media player, when there's no other source of timing.

Otherwise, ``roc_sender_write`` stores samples in a queue and returns immediately which could be usefull when sender's thread blocks on acquiring the samples, e.g. in `the module for pulseaudio <https://github.com/roc-project/pulseaudio-roc>`_.

The example didn't disable timing because it generates samples online, so it needs to be blocked so as not to oveflow roc's senders queue.
