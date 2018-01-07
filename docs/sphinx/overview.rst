.. _overview:

Overview
********

Terms
=====

sender
	is an application which sends audio-data via network to a receiver.

receiver
	is an application that receives packets with audio-data and process the audio samples somehow (e.g. plays or stores it to a file).

audio sample
	is a value of audio signal in a particular moment of time. An array of samples represents how sound pressure changes during the time.

audio channel
	is an audio signal dimension. For example stereo sound consists of two channels: left and right.

Feature list
============

Forward Erasure Correction Codes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Roc is being designed with an idea of sensible latency minimization in sight. That's why roc transmits audio content in UDP packets and why roc incorporates Forward Erasure Correction Codes.

Roc cuts samples flow into blocks and sends several redundant packets along with them so as to recover lost packets. In the example above, roc adds 5 redundant packets to every 10 data packets, so that the roc-recv is able to recover 5 data packets at maximum if they get lost or delayed. On the other hand the data rate is increased in 15/10 times.

Roc doesn't make FEC on its own: it uses `OpenFEC <http://openfec.org/>`_ for that purpose. OpenFEC provides two FEC schemes: Reed-Solomon and LDPC, the former one is more suitable for relatively small latency and small data-rates, therefore it is a default option.

Moreover, the roc's interface of block codec allows attaching another implementation with ease. Feel free to integrate great opensource and free implementation of some effective code.

Sender timing
^^^^^^^^^^^^^

One of the valuable feature of roc is that a receiver (e.g. **roc-recv**) adjusts its sampling frequency to a sampling rate of a sender. That's the reason why a sender must send packets with steady rate. In other words it's forbidden to send an entire .wav file at once. There're many other ways for that.

As packets must be scheduled carefully, roc could do this job for sender. Unless a user turns on ``ROC_API_CONF_DISABLE_TIMING`` option in ``roc_config::options``, ``roc_sender_write`` will block sender's thread until the samples are sent. This variant could be used in e.g. media player, when there's no other source of timing.

Otherwise, ``roc_sender_write`` stores samples in a queue and returns immediately which could be usefull when sender's thread blocks on acquiring the samples, e.g. in `the module for pulseaudio <https://github.com/roc-project/pulseaudio-roc>`_.

The example didn't disable timing because it generates samples online, so it needs to be blocked so as not to oveflow roc's senders queue.

Data format
^^^^^^^^^^^

Roc works with floating-point interleaved PCM samples in native endian ordering. ``roc_sender_write`` and ``roc_receiver_read`` both accept an array of floats with two interleaved channels -- Left-Right-Left-Right-etc.

In future roc is going to support other samples format and configurable channels set.
