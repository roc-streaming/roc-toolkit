Overview
********

.. contents:: Table of contents:
   :local:
   :depth: 1

What is it?
-----------

Roc Toolkit implements real-time audio streaming over the network.

Basically, it is a network transport, highly specialized for the real-time streaming. You write the stream to the one end and read it from another end, and Roc handles all the complexities of delivering data in time and with no loss. Encoding, decoding, maintaining latency, adjusting clocks, restoring losses -- all this happens transparently under the hood.

The project is conceived as a swiss army knife for real-time streaming. It is designed to support a variety of network protocols, encodings, FEC schemes, and related features. Users can build custom configurations dedicated for specific use cases, while balancing quality, robustness, bandwidth, and compatibility.

What problems does it solve?
----------------------------

.. seealso:: :doc:`/about_project/features`

Real-time streaming needs a special approach. When we create software for it, we must address several problems.

When the network is not reliable (e.g. Wi-Fi or Internet), we need to handle losses. We can't just use a reliable protocol like TCP, because the latency would suffer. We also can't just ignore losses, because the service quality would suffer.

When network conditions are not known beforehand, or are varying in time, we should adapt to changes dynamically. In response to changed conditions, we may need to adjust latency, compression level, redundancy level, and other parameters on fly.

When both sender input and receiver output have real-time clocks (e.g. both are sound cards), we must compensate the drift between their clocks. Otherwise, the sender and receiver will quickly get out of sync.

When sender and receiver capabilities differ, we need to negotiate stream format and select what is supported by everyone. When local and network formats are different, we should automatically perform all necessary conversions.

The real-time requirement leads to a compromise between many factors like quality, robustness, latency, bandwidth, and resource consumption. This compromise highly depends on the use-case. We should offer the user enough variety of protocols, codecs, and profiles, to suit everyone's needs.

Many applications that need real-time streaming often need similar related features. Encryption, QoS control, playback synchronization, to name a few. Such features are also in the scope of this project.

What are project goals?
-----------------------

At the very high level, the project has the following goals:

* *guaranteed latency* --- implement real-time streaming with guaranteed latency;
* *high quality streaming* --- support CD- and DVD-quality audio streams;
* *robust playback* --- provide good quality of service even on unreliable networks;
* *high-level and easy to use interface* --- implement a simple and high-level API that hides all the complexity from user;
* *work "from the box" with optional tuning* --- provide good defaults that work in many environments; and allow fine tuning to achieve best results for specific setup;
* *adaptive algorithms* --- support adjusting to environment conditions on fly;
* *comprehensive toolset* --- support multiple protocols and codecs, and give the user the full control of them;
* *portability* --- support multiple operating systems and hardware architectures;
* *interoperability* --- rely on open, standard protocols.

When do I need it?
------------------

You need Roc when you need streaming:

* with fixed or bounded latency;
* over unreliable network (e.g. Wi-Fi or Internet);
* between real-time sources (e.g. from captured output to sound card).

Example applications:

* live broadcasting software
* home audio systems and home cinema
* concert or hall sound systems
* online jamming (playing music together remotely)
* streaming platforms for radio
* cloud streaming

How do I use it?
----------------

.. seealso:: :doc:`/about_project/publications`

You can use the project several ways, depending on your use case:

* If you're creating an application, refer to :doc:`C library </api>` and its :doc:`bindings for other languages </api/bindings>`.

* Use :doc:`command-line tools </tools/command_line_tools>` in simple scenarios and for experimenting.

* Use :doc:`sound server modules </tools/sound_server_modules>` for deeper integration with systems like PulseAudio and PipeWire.

* For use on mobile, check out :doc:`end-user applications </tools/applications>`.

Using for home audio
~~~~~~~~~~~~~~~~~~~~

One application of Roc is building your own wireless home audio system.

Using Roc, you can interconnect audio applications and devices across different platforms and audio systems, for example, ALSA (using command-line tools), PulseAudio and PipeWire (using sound server modules), macOS CoreAudio (using virtual device), and Android (using mobile app).

Here are some examples of what you can do:

* you can connect speakers to a single-board computer and stream sound from your other devices to it;
* you can stream sound to your Android device, or back;
* you can set up some music server with remote control (like MPD) and then stream sound from it to other devices;
* you can set up several devices and switch between them on the fly via standard PulseAudio GUI or CLI; you can also route different apps to different devices;
* all of these can work over Wi-Fi with acceptable service quality.
