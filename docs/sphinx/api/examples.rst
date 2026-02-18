Examples
********

.. contents:: Table of contents:
   :local:
   :depth: 2

How to run
----------

Every linked example is a complete self-sufficient program, which you can build in one command (exact command is provided in the header comment of each example). It typically looks like this:

.. code::

   cc -o basic_sender_sine_wave basic_sender_sine_wave.c -lroc -lm

Then you can run example like this:

.. code::

   ./basic_sender_sine_wave

Alternatively, you can follow instructions from :doc:`/building/user_cookbook` to build the project, but add ``--enable-examples`` flag to scons invocation. All examples are then built automatically and put to the output binary directory.

Basic senders and receivers
---------------------------

.. note::

   Examples in this group use the same set of hard-coded IP (localhost) and ports. If you peek any pair of ``basic_sender_xxx`` and ``basic_receiver_xxx`` examples and run them in two terminals, you should see in logs that the sender has connected to the receiver.

.. cssclass:: api_examples

* **Sender (beep)** (`basic_sender_sine_wave.c <https://github.com/roc-streaming/roc-toolkit/blob/master/src/public_api/examples/basic_sender_sine_wave.c>`_)

  Minimal sender:
   - creates a sender and connects it to remote address
   - generates a 10-second beep and writes it to the sender

* **Sender (PulseAudio)** (`basic_sender_pulseaudio.c <https://github.com/roc-streaming/roc-toolkit/blob/master/src/public_api/examples/basic_sender_pulseaudio.c>`_)

  Another minimal sender:
   - creates a sender and connects it to remote address
   - captures audio stream from PulseAudio source and writes it to the sender

* **Receiver (WAV file)** (`basic_receiver_wav_file.c <https://github.com/roc-streaming/roc-toolkit/blob/master/src/public_api/examples/basic_receiver_wav_file.c>`_)

  Minimal receiver:
   - creates a receiver and binds it to a local address
   - reads audio stream from the receiver and writes it to a WAV file

* **Receiver (PulseAudio)** (`basic_receiver_pulseaudio.c <https://github.com/roc-streaming/roc-toolkit/blob/master/src/public_api/examples/basic_receiver_pulseaudio.c>`_)

  Another minimal receiver:
   - creates a receiver and binds it to a local address
   - reads audio stream from the receiver and plays to PulseAudio sink

Advanced network configuration
------------------------------

.. note::

   Every example in this group contains both sender and receiver (or a few of them), running on different threads. So you can peek any of the ``send_recv_xxx`` examples and run it standalone.

.. cssclass:: api_examples

* **Bare RTP** (`send_recv_rtp.c <https://github.com/roc-streaming/roc-toolkit/blob/master/src/public_api/examples/send_recv_rtp.c>`_)

  Sending and receiving using bare RTP without extensions:
   - creates a receiver and binds it to a single RTP endpoint
   - creates a sender and connects it to the receiver endpoint
   - one thread writes audio stream to the sender
   - another thread reads audio stream from receiver

* **RTP + FECFRAME + RTCP** (`send_recv_rtp_rtcp_fec.c <https://github.com/roc-streaming/roc-toolkit/blob/master/src/public_api/examples/send_recv_rtp_rtcp_fec.c>`_)

  Sending a receiving using RTP + FECFRAME + RTCP.

  This example is like the previous one, but it uses three endpoints (on both sender and receiver):
   - source endpoint is used to transmit audio stream (via RTP)
   - repair endpoint is used to transmit redundant stream for loss recovery (via FECFRAME)
   - control endpoint is used to transmit bidirectional control traffic (via RTCP)

* **Multicast IP** (`send_recv_multicast.c <https://github.com/roc-streaming/roc-toolkit/blob/master/src/public_api/examples/send_recv_multicast.c>`_)

  This example shows how to send stream to multiple receivers using IP multicast:
   - creates two receivers and binds them to multicast endpoints
   - creates a sender and connects it to the multicast endpoints
   - one thread writes audio stream to the sender
   - another two threads read audio stream from receivers

* **One sender + two unicast receivers** (`send_recv_1_sender_2_receivers.c <https://github.com/roc-streaming/roc-toolkit/blob/master/src/public_api/examples/send_recv_1_sender_2_receivers.c>`_)

  This example shows how to use slots mechanism to connect sender to two different receivers using unicast addresses:
   - creates two receivers and binds each one to its own unicast address
   - creates a sender
   - connects slot 1 of the sender to the first receiver
   - connects slot 2 of the sender to the second receiver
   - one thread writes audio stream to the sender
   - another two threads read audio stream from receivers

* **Two senders + one receiver with two unicast addresses** (`send_recv_2_senders_1_receiver.c <https://github.com/roc-streaming/roc-toolkit/blob/master/src/public_api/examples/send_recv_2_senders_1_receiver.c>`_)

  This example shows how to use slots mechanism to bind receiver to two different addresses (for example on different network interfaces or using different network protocols), and then connect two senders to those address.
   - creates a receiver
   - binds slot 1 of the receiver to the first address, using bare RTP
   - binds slot 2 of the receiver to the second address, using RTP + FECFRAME + RTCP
   - creates two senders and connects each one to its own address of the receiver
   - two threads writes audio stream to the senders
   - another thread reads mixed audio stream from receiver

Custom plugins
--------------

.. cssclass:: api_examples

* **Packet loss concealment (PLC)** (`plugin_plc.c <https://github.com/roc-streaming/roc-toolkit/blob/master/src/public_api/examples/plugin_plc.c>`_)

  Packet loss concealment is used to reduce distortion caused by packet losses that can't be repaired by FEC, by filling gaps with interpolated data. This example shows how you can implement and register custom PLC implementation with your own interpolation algorithm.

Utility functions
-----------------

.. cssclass:: api_examples

* **Endpoint URIs** (`uri_manipulation.c <https://github.com/roc-streaming/roc-toolkit/blob/master/src/public_api/examples/uri_manipulation.c>`_)

  Demonstrates how to build endpoint URI and access its individual parts.

* **Logger settings** (`misc_logging.c <https://github.com/roc-streaming/roc-toolkit/blob/master/src/public_api/examples/misc_logging.c>`_)

  Demonstrates how to configure log level and log handler.

* **Version checks** (`misc_version.c <https://github.com/roc-streaming/roc-toolkit/blob/master/src/public_api/examples/misc_version.c>`_)

  Demonstrates how to check library version at compile-time and run-time.
