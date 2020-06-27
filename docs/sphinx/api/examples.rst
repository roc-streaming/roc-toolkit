Examples
********

.. contents:: Table of contents:
   :local:
   :depth: 2

Basic sender and receiver
-------------------------

<<<<<<< HEAD
* `basic_sender_sine_wave.c <https://github.com/roc-project/roc/blob/master/src/library/example/basic_sender_sine_wave.c>`_
=======
* `basic_sender_sine_wave.c <https://github.com/roc-streaming/roc-toolkit/blob/master/src/library/example/basic_sender_sine_wave.c>`_
>>>>>>> d8f74d5d3fb22f41808e9a1d19ad46742ca33476

  Basic sender example.

  This example creates a sender and connects it to remote receiver.
  Then it generates a 10-second beep and writes it to the sender.

<<<<<<< HEAD
* `basic_sender_from_pulseaudio.c <https://github.com/roc-project/roc/blob/master/src/library/example/basic_sender_from_pulseaudio.c>`_
=======
* `basic_sender_from_pulseaudio.c <https://github.com/roc-streaming/roc-toolkit/blob/master/src/library/example/basic_sender_from_pulseaudio.c>`_
>>>>>>> d8f74d5d3fb22f41808e9a1d19ad46742ca33476

  Basic sender example.

  This example creates a sender and connects it to remote receiver.
  Then it records audio stream from PulseAudio and writes it to the sender.

<<<<<<< HEAD
* `basic_receiver_to_pulseaudio.c <https://github.com/roc-project/roc/blob/master/src/library/example/basic_receiver_to_pulseaudio.c>`_
=======
* `basic_receiver_to_pulseaudio.c <https://github.com/roc-streaming/roc-toolkit/blob/master/src/library/example/basic_receiver_to_pulseaudio.c>`_
>>>>>>> d8f74d5d3fb22f41808e9a1d19ad46742ca33476

  Basic receiver example.

  This example creates a receiver and binds it to a known address.
  Then it reads audio stream from the receiver and plays it using PulseAudio.

Network protocols
-----------------

<<<<<<< HEAD
* `send_receive_rtp.c <https://github.com/roc-project/roc/blob/master/src/library/example/send_receive_rtp.c>`_
=======
* `send_receive_rtp.c <https://github.com/roc-streaming/roc-toolkit/blob/master/src/library/example/send_receive_rtp.c>`_
>>>>>>> d8f74d5d3fb22f41808e9a1d19ad46742ca33476

  Send and receive samples using bare RTP.

  This example creates a receiver and binds it to an RTP endpoint.
  Then it creates a sender and connects it to the receiver endpoint.
  Then it starts writing audio stream to the sender and reading it from receiver.

<<<<<<< HEAD
* `send_receive_rtp_with_fecframe.c <https://github.com/roc-project/roc/blob/master/src/library/example/send_receive_rtp_with_fecframe.c>`_
=======
* `send_receive_rtp_with_fecframe.c <https://github.com/roc-streaming/roc-toolkit/blob/master/src/library/example/send_receive_rtp_with_fecframe.c>`_
>>>>>>> d8f74d5d3fb22f41808e9a1d19ad46742ca33476

  Send and receive samples using RTP and FECFRAME.

  This example is like `send_receive_rtp.c`, but it creates two endpoints:
   - the first, source endpoint is used to transmit audio stream
   - the first, repair endpoint is used to transmit redundant stream

  The redundant stream is used on receiver to recover lost audio packets.
  This is useful on unreliable networks.

Miscellaneous
-------------

<<<<<<< HEAD
* `uri_manipulation.c <https://github.com/roc-project/roc/blob/master/src/library/example/uri_manipulation.c>`_
=======
* `uri_manipulation.c <https://github.com/roc-streaming/roc-toolkit/blob/master/src/library/example/uri_manipulation.c>`_
>>>>>>> d8f74d5d3fb22f41808e9a1d19ad46742ca33476

  URI manipulation example.

  This example demonstrates how to build endpoint URI and access its individual parts.
