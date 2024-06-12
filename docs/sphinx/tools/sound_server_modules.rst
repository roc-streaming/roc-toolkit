Sound server modules
********************

PulseAudio modules
==================

`roc-pulse <https://github.com/roc-streaming/roc-pulse>`_ repo implements Roc PulseAudio modules.

========================= ====================================================================
Module                    Description
========================= ====================================================================
``module-roc-sink-input`` receives audio from network, can be connected to local audio device
``module-roc-sink``       sends audio to network, local audio apps can be connected to it
========================= ====================================================================

Highlights:

* Compared to command-line tools, these modules better integrate with PulseAudio. You can connect roc receiver or sender to local device or app via usual PulseAudio GUI and CLI tools, for example ``pavucontrol``.

* Compared to builtin PulseAudio network transports ("native" and "rtp"), Roc modules provide better service quality (less losses and glitches) over unreliable networks like Wi-Fi.

* Compared to builtin PulseAudio network transports, Roc modules allow you to interconnect different systems. They are fully interoperable with other Roc receivers and senders based on libroc, e.g. Roc :doc:`command-line tools </tools/command_line_tools>` or :doc:`applications </tools/applications>`.

.. note::

   In previous releases, PulseAudio modules were part of ``roc-toolkit`` repo. Now they live in separate repo and have independent build system, documentation, and versioning.

PipeWire modules
================

PipeWire provides two Roc modules.

===================================================================== ====================================================================
Module                                                                Description
===================================================================== ====================================================================
`roc-source <https://docs.pipewire.org/page_module_roc_source.html>`_ receives audio from network, can be connected to local audio device
`roc-sink <https://docs.pipewire.org/page_module_roc_sink.html>`_     sends audio to network, local audio apps can be connected to it
===================================================================== ====================================================================

The idea is similar to Roc PulseAudio modules. Unlike PulseAudio modules, PipeWire modules are not part of Roc, but are provided by PipeWire itself.

macOS virtual device
====================

`Roc VAD <https://github.com/roc-streaming/roc-vad>`_ implements macOS driver for Virtual Audio Device (VAD).

You can create output device (virtual speakers) that streams all sound to remote peer. Similarly, you can create input device (virtual microphone) that receives sound from remote peers.

Driver can be configured from command-line, or programmatically via gRPC interface, which allows to use it in your own projects.

For further details, please refer to the project documentation.
