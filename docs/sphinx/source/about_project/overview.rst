Overview
********

.. seealso:: :doc:`/about_project/features`, :doc:`/about_project/usage`, :doc:`/about_project/publications`

.. contents:: Table of contents:
   :local:
   :depth: 1

Synopsis
--------

Roc is a toolkit for real-time audio streaming over the network.

Basically, Roc is a network transport, highly specialized for the real-time streaming use case. The user writes the stream to the one end and reads it from another end, and Roc deals with all the complexity of the task of delivering data in time and with no loss. Encoding, decoding, adjusting rates, restoring losses -- all these are performed transparently under the hood.

The project is conceived as a swiss army knife for real-time streaming. It is designed to support a variety of network protocols, encodings, FEC schemes, and related features. The user can build custom configurations dedicated for specific use cases and choose an appropriate compromise between the quality, robustness, bandwidth, and compatibility issues.

Note that the project is still at an early stage of development, and the list of the supported features is not very long. But there's more to come, of course. Contributions are also welcome!

Rationale
---------

Why real-time streaming needs a special approach?

First, if both sender input and receiver output have real-time clocks, we should somehow deal with the (small) difference between their frequencies because every device has its own clock domain. A real-time streaming library should solve this problem, or otherwise, the sender and receiver streams will quickly get out of sync.

Second, if the network is not reliable, we should cope with losses. We can't just retry lost packets until they are delivered because it may be too late when we're done retrying. A real-time streaming library should solve this problem as well, or otherwise either latency or quality of service will suffer.

Third, the real-time requirement usually leads to a compromise between various factors like quality, robustness, latency, bandwidth, and resource consumption. The right configuration depends on the specific use case. On the other hand, different encodings are suitable for different quality requirements, different FEC schemes are suitable for different latency and CPU requirements, and so on. Therefore, a real-time streaming library should support a variety of protocols, media encodings, FEC codes, and allow the user to select the most suitable configuration.

Fourth, applications that need real-time streaming usually also need some related features like service announcement and discovery, session negotiation, stream feedback and control, and so on. Such features are not directly related to real-time, but still would be expected to be found in a real-time streaming library.

All of the issues listed above belong to the scope of this project. Some of them are addressed already, and others will be addressed in future releases.

Project goals
-------------

Project goals:

* *real-time* --- allow the user to create real-time streams with guaranteed latency;
* *good media quality* --- cope even with medium- and high-quality audio streams;
* *good service quality* --- provide good quality of service even on unreliable networks;
* *high-level* --- implement a simple high-level API that hides all the complexity of the network transport;
* *comprehensive toolset* --- support multiple protocols, encodings, schemes, and give the user the full control of them;
* *portability* --- support multiple operating systems and hardware architectures;
* *interoperability* --- rely on open, standard protocols.

Project scope
-------------

The following list can help to determine whether Roc is suitable for a specific use case.

Latency requirements:

* low latency --- yes
* higher latency --- yes
* not real-time --- not very helpful, better use protocols like HTTP

Quality requirements:

* lossless --- yes; CD-quality currently
* lossy --- not yet supported, but will be
* HD --- not yet supported, but will be

Network type:

* wireless LAN --- yes
* wired LAN --- yes, still useful even though the network is reliable
* Internet --- yes, but no encryption is implemented yet

Media type:

* audio --- yes
* video --- not yet supported, but will be
* other --- not supported; maybe in future, but no plans yet

Platform type:

* server --- yes
* desktop -- yes, but we should separate desktop integrations from core
* mobile --- not yet supported, but will be
* embedded --- only Linux is supported currently

Networking features:

* network I/O --- yes
* real-time streaming protocols --- yes; only RTP-based protocols are implemented so far
* control protocols --- not yet supported, but will be

Sound processing features:

* sound I/O --- not yet (except command-line tools); use other libraries
* DSP --- limited; Roc is not a general-purpose DSP library

Application fields
------------------

Below is the list of example applications where Roc could be useful.

Some of the applications will be possible only after adding relevant features like service discovery or video support, which are not ready yet. If you are thinking about using Roc in an open-source or closed-source project and want to join the efforts or need some improvements in Roc, feel free to contact us.

Example applications:

* live broadcasting software
* cloud streaming
* VoIP, teleconferences
* home audio systems and home cinema
* mobile audio/video sharing
* video surveillance
* remote desktop
* remote controls with cameras
