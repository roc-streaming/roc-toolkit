PulseAudio modules
****************** 

Overview
========

Roc provides a set of `PulseAudio <https://www.freedesktop.org/wiki/Software/PulseAudio/>`_ modules allowing PulseAudio to use Roc as a transport and improve its service quality over an unreliable network such as 802.11.

.. warning::

   Work in progress! The implementation is in the proof-of-concept stage. Not all features are implemented, and implemented features are not fully tested yet. Most notably, there is no service discovery, so that the user have to register a pair of sink and sink-input manually. It's planned to be implemented in upcoming releases.

Advantages over Roc command-line tools:

- Seamless integration into the PulseAudio workflow. The user can connect a local audio stream to a remote audio device using common PulseAudio tools.

- A bit lower latency. Since Roc is integrated into the PulseAudio server, there is no additional communication step between Roc and PulseAudio server.

Advantages over PulseAudio "native" protocol:

- Accurate latency. PulseAudio "native" protocol uses TCP, while Roc uses RTP, which is better suited for real-time communication than TCP-based protocols.

- Compatibility with standard protocols. PulseAudio "native" protocol is PulseAudio-specific, while Roc implements a set of standardized RTP-based protocols.

Advantages over PulseAudio builtin RTP support:

- Better service quality over an unreliable network. This is achieved by employing Forward Erasure Correction codes.

Building
========

You need to build PulseAudio itself from sources (either automatically or manually) and enable building Roc PulseAudio modules using ``--enable-pulseaudio-modules`` SCons option.

.. warning::

   You should build exactly the same PulseAudio version as you're running since PulseAudio does not provide an ABI and API compatibility for internal modules.

To download and build PulseAudio automatically, use ``--build-3rdparty`` SCons option:

.. code::

   $ scons --enable-pulseaudio-modules --build-3rdparty=pulseaudio ...       # auto-detect PulseAudio version
   $ scons --enable-pulseaudio-modules --build-3rdparty=pulseaudio:10.0 ...  # use specified PulseAudio version

Alternatively, you can `build PulseAudio manually <https://www.freedesktop.org/wiki/Software/PulseAudio/Documentation/Developer/PulseAudioFromGit/>`_ and use ``--with-pulseaudio`` option to specify a directory where PulseAudio sources are located:

.. code::

   $ scons --enable-pulseaudio-modules --with-pulseaudio=/path/to/pulseaudio ...

See our :doc:`/building` page for details. In particular, :doc:`/building/quick_start` page provides building examples for popular distros.

Installing
==========

Copy (or symlink) Roc PulseAudio modules to the PulseAudio modules directory, e.g.:

.. code::

   $ cp ./bin/x86_64-pc-linux-gnu/module-roc-sink.so /usr/lib/pulse-10.0/modules/
   $ cp ./bin/x86_64-pc-linux-gnu/module-roc-sink-input.so /usr/lib/pulse-10.0/modules/

Copy (or symlink) Roc C library to the system library directory, e.g.:

.. code::

   $ cp ./bin/x86_64-pc-linux-gnu/libroc.so /usr/lib/

Sender
======

For the sending side, Roc provides ``module-roc-sink`` PulseAudio module. It creates a PulseAudio sink that sends samples written to it to a preconfigured receiver address. You can then connect an audio stream of any running application to that sink, or make it the default sink.

Roc sink supports several options:

===================== ======== ============== ==========================================
option                required default        description
===================== ======== ============== ==========================================
sink_name             no       roc_sender     the name of the new sink
sink_properties       no                      additional sink properties
local_ip              no       0.0.0.0        local sender address to bind to
remote_ip             yes                     remote receiver address
remote_source_port    no       10001          remote receiver port for source (audio) packets
remote_repair_port    no       10002          remote receiver port for repair (FEC) packets
===================== ======== ============== ==========================================

Here is how you can create a Roc sink from command line:

.. code::

   $ pactl load-module module-roc-sink remote_ip=<receiver_ip>

Alternatively, you can add this line to ``/etc/pulse/default.pa`` to create a Roc sink automatically at PulseAudio start:

.. code::

   load-module module-roc-sink remote_ip=<receiver_ip>

You can then connect an audio stream (i.e. a sink input) to the Roc sink via command line:

.. code::

   $ pactl move-sink-input <sink_input_number> roc_sender

Or via the ``pavucontrol`` graphical tool:

.. image:: ../../screenshots/roc_pulse_sender.png
    :width: 600px

Receiver
========

For the receiving side, Roc provides ``module-roc-sink-input`` PulseAudio module. It creates a PulseAudio sink input that receives samples from Roc sender and passes them to the sink it is connected to. You can then connect it to any audio device.

Roc sink input supports several options:

===================== ======== ============== ==========================================
option                required default        description
===================== ======== ============== ==========================================
sink                  no       <default sink> the name of the sink to connect the new sink input to
sink_input_properties no                      additional sink input properties
local_ip              no       0.0.0.0        local address to bind to
local_source_port     no       10001          local port for source (audio) packets
local_repair_port     no       10002          local port for repair (FEC) packets
===================== ======== ============== ==========================================

Here is how you can create a Roc sink from command line:

.. code::

   $ pactl load-module module-roc-sink-input

Alternatively, you can add this line to ``/etc/pulse/default.pa`` to create a Roc sink automatically at PulseAudio start:

.. code::

   load-module module-roc-sink-input

You can then connect the Roc sink input to an audio device (i.e. a sink) via command line:

.. code::

   # determine Roc sink-input number
   $ pactl list sink-inputs

   # connect Roc sink-input to a sink
   $ pactl move-sink-input <roc_sink_input_number> <sink>

Or via the ``pavucontrol`` graphical tool:

.. image:: ../../screenshots/roc_pulse_receiver.png
    :width: 600px

Interoperability
================

Roc PulseAudio modules are interoperable with Roc library command line tools, i.e.:

- as a sender, you can use either ``roc_sender`` from the C library, ``roc-send`` command line tool, or ``module-roc-sink``

- as a receiver, you can use either ``roc_receiver`` from the C library, ``roc-recv`` command line tool, or ``module-roc-sink-input``

Troubleshooting
===============

First, run PulseAudio server in verbose mode, both on sending and receiving sides:

.. code::

   $ pulseaudio -vvv

Among other things, you should find some messages from Roc sink and sink-input there, which may give some idea about what's going wrong.

Second, you can try to replace sender, receiver, or both with Roc command line tools to determine whether the issue is specific to PulseAudio modules or not.
