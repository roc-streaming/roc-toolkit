Examples
********

.. contents:: Table of contents:
   :local:
   :depth: 2

Basic sender and receiver
-------------------------

* `basic_sender_sine_wave.c <https://github.com/roc-streaming/roc-toolkit/blob/master/src/public_api/examples/basic_sender_sine_wave.c>`_

  Basic sender example.

  This example creates a sender and connects it to remote receiver.
  Then it generates a 10-second beep and writes it to the sender.

* `basic_sender_from_pulseaudio.c <https://github.com/roc-streaming/roc-toolkit/blob/master/src/public_api/examples/basic_sender_from_pulseaudio.c>`_

  Basic sender example.

  This example creates a sender and connects it to remote receiver.
  Then it records audio stream from PulseAudio and writes it to the sender.

* `basic_receiver_to_pulseaudio.c <https://github.com/roc-streaming/roc-toolkit/blob/master/src/public_api/examples/basic_receiver_to_pulseaudio.c>`_

  Basic receiver example.

  This example creates a receiver and binds it to a known address.
  Then it reads audio stream from the receiver and plays it using PulseAudio.

Network protocols
-----------------

* `send_receive_rtp.c <https://github.com/roc-streaming/roc-toolkit/blob/master/src/public_api/examples/send_receive_rtp.c>`_

  Send and receive samples using bare RTP.

  This example creates a receiver and binds it to an RTP endpoint.
  Then it creates a sender and connects it to the receiver endpoint.
  Then it starts writing audio stream to the sender and reading it from receiver.

* `send_receive_rtp_with_fecframe.c <https://github.com/roc-streaming/roc-toolkit/blob/master/src/public_api/examples/send_receive_rtp_with_fecframe.c>`_

  Send and receive samples using RTP and FECFRAME.

  This example is like `send_receive_rtp.c`, but it creates two endpoints:
   - the first, source endpoint is used to transmit audio stream
   - the second, repair endpoint is used to transmit redundant stream

  The redundant stream is used on receiver to recover lost audio packets.
  This is useful on unreliable networks.

Miscellaneous
-------------

* `uri_manipulation.c <https://github.com/roc-streaming/roc-toolkit/blob/master/src/public_api/examples/uri_manipulation.c>`_

  URI manipulation example.

  This example demonstrates how to build endpoint URI and access its individual parts.
