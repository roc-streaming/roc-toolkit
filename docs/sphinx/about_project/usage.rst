Usage
*****

Using Roc as a library
======================

.. seealso:: :doc:`/api`

Roc comes with a general-purpose C API which provides a real-time transport for applications.

Consider using Roc library when:

* You're writing an audio application and need a real-time network transport that "just works". Roc will provide you with a simple API that hides all the complexity of real-time delivery.

* You need fixed latency and good service quality over an unreliable network at the same time. Bare RTP will give you the former, but not the latter; TCP or HTTP will give you the latter, but not the former; Roc will give you both.

* You want to stream high-quality lossless audio with loss recovery. Opus, for example, has loss recovery, but it's a lossy codec; Roc can employ encoding-independent loss recovery and lossless encodings.

* Your source and destination both have their own clocks, in particular when you want to connect two audio devices. Roc will automatically perform the conversion between the clock domains for you.

Using Roc for home audio
========================

.. seealso:: :doc:`/about_project/publications`

One possible application of Roc is building your own wireless home audio system.

In addition to the C API, Roc provides command-line tools and PulseAudio modules which can be used for this use case.

Consider using them when:

* You want to connect audio applications and devices over an unreliable network like Wi-Fi and want live streaming with a fixed latency and yet good service quality. Most software will not give you both.

* You want to interconnect audio applications and devices across different platforms and audio systems, for example, ALSA, PulseAudio, and macOS CoreAudio.

* You want to improve PulseAudio service quality over an unreliable network like Wi-Fi.

* You want a network transport that transparently integrates into the PulseAudio workflow and can be controlled via the usual PulseAudio GUIs.

Here are some examples of what you can do with Roc:

* you can connect speakers to a single-board computer and stream sound from your other devices to it;

* you can stream sound to your Android device;

* you can set up some music server with remote control (like MPD) and then stream sound from it to other devices;

* you can set up several devices and switch between them on the fly via standard PulseAudio GUI or CLI; you can also route different apps to different devices;

* all of these can work over Wi-Fi with acceptable service quality.
